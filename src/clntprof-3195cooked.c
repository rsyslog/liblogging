/*! \file clntprof-3195cooked.c
 *  \brief Implementation of the client profile for RFC 3195 COOKED.
 *
 * The prefix for this "object" is psrr which stands for
 * *P*rofile *S*yslog *R*eliable *Cooked*. This file works in
 * conjunction with \ref lstnprof-3195cooked.c and shares
 * its namespace.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-09-05
 *          coding begun.
 *
 * Copyright 2002-2003 
 *     Rainer Gerhards and Adiscon GmbH. All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 *     * Neither the name of Adiscon GmbH or Rainer Gerhards
 *       nor the names of its contributors may be used to
 *       endorse or promote products derived from this software without
 *       specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include "config.h"
#include "liblogging.h"
#include "srAPI.h"
#include "beepsession.h"
#include "beepchannel.h"
#include "beepprofile.h"
#include "clntprof-3195raw.h"
#include "namevaluetree.h"
#include "stringbuf.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */

/**
 * Construct a sbPSSRObj.
 */
static srRetVal sbPSRCConstruct(sbPSSRObj** ppThis)
{
	assert(ppThis != NULL);

	if((*ppThis = calloc(1, sizeof(sbPSSRObj))) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	(*ppThis)->OID = OIDsbPSSR;
	(*ppThis)->uAnsno = 0;
	(*ppThis)->uMsgno4raw = 0;

	return SR_RET_OK;
}


/**
 * Destroy a sbPSSRObj.
 * The handler must be called in the ChanClose handler, if instance
 * data was assigned.
 */
static void sbPSRCDestroy(sbPSSRObj* pThis)
{
	sbPSSRCHECKVALIDOBJECT(pThis);
	SRFREEOBJ(pThis);
}


/**
 * Wait for an "ok" reply from the remote peer. This reply is
 * needed for many situations, e.g. after sending the <iam> or
 * <entry> elements. As such, we have put it up in its own 
 * method.
 */
static srRetVal sbPSRCClntWaitOK(sbChanObj* pChan)
{
	srRetVal iRet;
	sbMesgObj* pMesgReply;
	sbNVTRObj *pReplyXML;

	sbChanCHECKVALIDOBJECT(pChan);
	
	if((pMesgReply = sbMesgRecvMesg(pChan)) == NULL)
		return SR_RET_ERR_RECEIVE;

	if(pMesgReply->idHdr == BEEPHDR_RPY)
		; /* empty be intension - this is OK! */
	else if(pMesgReply->idHdr == BEEPHDR_ERR)
	{
		sbMesgDestroy(pMesgReply);
		return SR_RET_PEER_INDICATED_ERROR;
	}
	else
	{
		sbMesgDestroy(pMesgReply);
		return SR_RET_UNEXPECTED_HDRCMD;
	}

	pReplyXML = sbNVTRConstruct();
	if((iRet = sbNVTRParseXML(pReplyXML, pMesgReply->szActualPayload)) == SR_RET_OK)
		if(sbNVTRHasElement(pReplyXML, "ok", TRUE) == NULL)
			iRet = SR_RET_PEER_NONOK_RESPONSE;

	sbNVTRDestroy(pReplyXML);
	sbMesgDestroy(pMesgReply);

	return iRet;
}


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */

srRetVal sbPSRCClntSendMsg(sbChanObj* pChan, char* szLogmsg)
{
	srRetVal iRet;
	sbMesgObj *pMesg;
	sbPSSRObj *pThis;
	sbStrBObj *pStrBuf;
	char *pMsg;

	sbChanCHECKVALIDOBJECT(pChan);
	assert(szLogmsg != NULL);

	if((pStrBuf = sbStrBConstruct()) == NULL)	return SR_RET_OUT_OF_MEMORY;

	/* build message */
	if((iRet = sbStrBAppendStr(pStrBuf, "<entry facility='7' severity='0' hostname='wsrger' deviceFQDN='rger.adiscon' deviceIP='172.19.1.20' timestamp='Sep  8 15:05:00'>")) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendStr(pStrBuf, sbNVTXMLEscapePCDATA(szLogmsg))) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendStr(pStrBuf, "</entry>")) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	pMsg = sbStrBFinish(pStrBuf);

	/* send message */
	pThis = pChan->pProfInstance;
	sbPSSRCHECKVALIDOBJECT(pThis);

	if((pMesg = sbMesgConstruct(NULL, pMsg)) == NULL)
		return SR_RET_ERR;

	iRet = sbMesgSendMesg(pMesg, pChan, "MSG", 0);
	sbMesgDestroy(pMesg);
	free(pMsg);

	/* message sent, now let's wait for the reply... */
	iRet = sbPSRCClntWaitOK(pChan);

	return iRet;
}


srRetVal sbPSRCClntOpenLogChan(sbChanObj *pChan, sbMesgObj *pMesgGreeting)
{
	srRetVal iRet;
	sbPSSRObj *pThis;
	sbMesgObj *pMesg;
	char *pBuf;
	char *pHostName;
	char fmtBuf[1024];

	sbChanCHECKVALIDOBJECT(pChan);

	if((iRet = sbPSRCConstruct(&pThis)) != SR_RET_OK)
		return iRet;

	if((iRet = sbSockGetIPusedForSending(pChan->pSess->pSock, &pBuf)) != SR_RET_OK)
	{
		sbPSRCDestroy(pThis);
		return iRet;
	}

	if((iRet = sbSock_gethostname(&pHostName)) != SR_RET_OK)
	{
		sbPSRCDestroy(pThis);
		return iRet;
	}

	SNPRINTF(fmtBuf, sizeof(fmtBuf), "<iam fqdn='%s' ip='%s' type='device'/>", pHostName, pBuf);
	if((pMesg = sbMesgConstruct("Content-type: application/beep+xml\r\n",
		fmtBuf)) == NULL)
		return SR_RET_ERR;

	iRet = sbMesgSendMesg(pMesg, pChan, "MSG", 0);
	sbMesgDestroy(pMesg);
	free(pBuf);
	free(pHostName);

	/* message sent, now let's wait for the reply... */
	iRet = sbPSRCClntWaitOK(pChan);

	/* ok, all gone well, now store the instance pointer */
	pChan->pProfInstance = pThis;
    
	return SR_RET_OK;
}


srRetVal sbPSRCCOnClntCloseLogChan(sbChanObj *pChan)
{
	sbPSSRObj *pThis;

	sbChanCHECKVALIDOBJECT(pChan);

	pThis = pChan->pProfInstance;
	sbPSSRCHECKVALIDOBJECT(pThis);

	/* In this profile, we do not need to do any
	 * profile-specific shutdown messages. All is
	 * done by the underlying BEEP channel management.
	 */

	sbPSRCDestroy(pThis);
	pChan->pProfInstance = NULL;

	return SR_RET_OK;
}
