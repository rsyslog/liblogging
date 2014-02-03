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
 * Copyright 2002-2014 
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
#include "settings.h"
#include "liblogging.h"
#include "srAPI.h"
#include "beepsession.h"
#include "beepchannel.h"
#include "beepprofile.h"
#include "clntprof-3195cooked.h"
#include "namevaluetree.h"
#include "stringbuf.h"
#include "syslogmessage.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */

/**
 * Construct a sbPSRCObj.
 */
static srRetVal sbPSRCConstruct(sbPSRCObj** ppThis)
{
	assert(ppThis != NULL);

	if((*ppThis = calloc(1, sizeof(sbPSRCObj))) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	(*ppThis)->OID = OIDsbPSRC;
	(*ppThis)->uNextMsgno = 0;
	(*ppThis)->pszMyIP = NULL;
	(*ppThis)->pszMyHostName = NULL;

	return SR_RET_OK;
}


/**
 * Destroy a sbPSSRObj.
 * The handler must be called in the ChanClose handler, if instance
 * data was assigned.
 */
static void sbPSRCDestroy(sbPSRCObj* pThis)
{
	sbPSRCCHECKVALIDOBJECT(pThis);

	if(pThis->pszMyIP != NULL)
			free(pThis->pszMyIP);
	if(pThis->pszMyHostName != NULL)
			free(pThis->pszMyHostName);

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

/** 
 * Handler to send a srSLMGObj to the remote peer. This
 * is the preferred way to send things - with COOKED, we would
 * otherwise need to re-parse the message just to obtain the
 * information that the caller most probably already has...
 */
srRetVal sbPSRCClntSendSLMG(sbChanObj* pChan, srSLMGObj *pSLMG)
{
	srRetVal iRet;
	sbMesgObj *pMesg;
	sbPSRCObj *pThis;
	sbStrBObj *pStrBuf;
	char *pMsg;

	sbChanCHECKVALIDOBJECT(pChan);
	srSLMGCHECKVALIDOBJECT(pSLMG);

	pThis = pChan->pProfInstance;
	sbPSRCCHECKVALIDOBJECT(pThis);

	if((pStrBuf = sbStrBConstruct()) == NULL)	{ srSLMGDestroy(pSLMG); return SR_RET_OUT_OF_MEMORY; }

	/* build message */
	/* begin & facility */
	if((iRet = sbStrBAppendStr(pStrBuf, "<entry facility='")) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendInt(pStrBuf, pSLMG->iFacility)) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendChar(pStrBuf, '\'')) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }

	/* severity */
	if((iRet = sbStrBAppendStr(pStrBuf, " severity='")) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendInt(pStrBuf, pSLMG->iSeverity)) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendChar(pStrBuf, '\'')) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }

	/* timestamp */
	if((iRet = sbStrBAppendStr(pStrBuf, " timestamp='")) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendStr(pStrBuf, pSLMG->pszTimeStamp)) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendChar(pStrBuf, '\'')) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }

	/* hostname */
	if((iRet = sbStrBAppendStr(pStrBuf, " hostname='")) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendStr(pStrBuf, pSLMG->pszHostname)) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendChar(pStrBuf, '\'')) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }

	/* tag (if available) */
	if(pSLMG->pszTag != NULL)
	{
		if((iRet = sbStrBAppendStr(pStrBuf, " tag='")) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
		if((iRet = sbStrBAppendStr(pStrBuf, pSLMG->pszTag)) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
		if((iRet = sbStrBAppendChar(pStrBuf, '\'')) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	}

	/* deviceFQDN */
	if((iRet = sbStrBAppendStr(pStrBuf, " deviceFQDN='")) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendStr(pStrBuf, pSLMG->pszHostname)) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendChar(pStrBuf, '\'')) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }

	/* deviceIP */
	if((iRet = sbStrBAppendStr(pStrBuf, " deviceIP='")) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendStr(pStrBuf, pThis->pszMyIP)) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendChar(pStrBuf, '\'')) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }

	/* end of <entry> and data fields */
	if((iRet = sbStrBAppendChar(pStrBuf, '>')) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }

	if((iRet = sbNVTXMLEscapePCDATAintoStrB(pSLMG->pszRawMsg, pStrBuf)) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	if((iRet = sbStrBAppendStr(pStrBuf, "</entry>")) != SR_RET_OK) {sbStrBDestruct(pStrBuf); return iRet; }
	pMsg = sbStrBFinish(pStrBuf);

	/* send message */
	if((pMesg = sbMesgConstruct(NULL, pMsg)) == NULL)
		return SR_RET_ERR;
	free(pMsg);

	iRet = sbMesgSendMesg(pMesg, pChan, "MSG", 0);
	sbMesgDestroy(pMesg);

	if(iRet != SR_RET_OK)
		return iRet;

	/* message sent, now let's wait for the reply... */
	iRet = sbPSRCClntWaitOK(pChan);

	return iRet;
}

srRetVal sbPSRCClntSendMsg(sbChanObj* pChan, char* szLogmsg)
{
	srRetVal iRet;
	srSLMGObj *pSLMG;

	sbChanCHECKVALIDOBJECT(pChan);
	assert(szLogmsg != NULL);

	/* OK, if we receive a raw message, we must parse it back so 
	 * that we have the properties at hand. Probably this mode should
	 * be removed, but who knows... It is better in any case to use
	 * the SendSLMG() call...
	 */
	if((iRet = srSLMGConstruct(&pSLMG)) != SR_RET_OK)	return iRet;
	if((iRet = srSLMGSetRawMsg(pSLMG, szLogmsg, FALSE)) != SR_RET_OK) { srSLMGDestroy(pSLMG); return iRet; }
	if((iRet = srSLMGParseMesg(pSLMG)) != SR_RET_OK) { srSLMGDestroy(pSLMG); return iRet; }

	iRet = sbPSRCClntSendSLMG(pChan, pSLMG);
	srSLMGDestroy(pSLMG);

	return iRet;
}


srRetVal sbPSRCClntOpenLogChan(sbChanObj *pChan)
{
	srRetVal iRet;
	sbPSRCObj *pThis;
	sbMesgObj *pMesg;
	char fmtBuf[1024];

	sbChanCHECKVALIDOBJECT(pChan);

	if((iRet = sbPSRCConstruct(&pThis)) != SR_RET_OK)
		return iRet;

	if((iRet = sbSockGetIPusedForSending(pChan->pSess->pSock, &(pThis->pszMyIP))) != SR_RET_OK)
	{
		sbPSRCDestroy(pThis);
		return iRet;
	}

	if((iRet = sbSock_gethostname(&(pThis->pszMyHostName))) != SR_RET_OK)
	{
		sbPSRCDestroy(pThis);
		return iRet;
	}

	SNPRINTF(fmtBuf, sizeof(fmtBuf), "<iam fqdn='%s' ip='%s' type='device' />", pThis->pszMyHostName, pThis->pszMyIP);
	if((pMesg = sbMesgConstruct("Content-type: application/beep+xml\r\n",
		fmtBuf)) == NULL)
		return SR_RET_ERR;

	iRet = sbMesgSendMesg(pMesg, pChan, "MSG", 0);
	sbMesgDestroy(pMesg);

	/* message sent, now let's wait for the reply... */
	iRet = sbPSRCClntWaitOK(pChan);

	/* ok, all gone well, now store the instance pointer */
	pChan->pProfInstance = pThis;
    
	return SR_RET_OK;
}


srRetVal sbPSRCCOnClntCloseLogChan(sbChanObj *pChan)
{
	sbPSRCObj *pThis;

	sbChanCHECKVALIDOBJECT(pChan);

	pThis = pChan->pProfInstance;
	sbPSRCCHECKVALIDOBJECT(pThis);

	/* In this profile, we do not need to do any
	 * profile-specific shutdown messages. All is
	 * done by the underlying BEEP channel management.
	 */

	sbPSRCDestroy(pThis);
	pChan->pProfInstance = NULL;

	return SR_RET_OK;
}
