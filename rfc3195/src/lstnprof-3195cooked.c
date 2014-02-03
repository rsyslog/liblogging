/*! \file lstnprof-3195cooked.c
 *  \brief Implementation of the listener profile for RFC 3195 COOKED.
 *
 * The prefix for this "object" is psrc which stands for
*  *P*rofile *S*yslog *R*eliable *Cooked*.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-09-05
 *          file creatd, but coding not yet begun
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
#include "beepprofile.h"
#include "lstnprof-3195cooked.h"
#include "stringbuf.h"
#include "syslogmessage.h"
#include "namevaluetree.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */

/**
 * Send an <ok/> reply to the remote peer.
 * This indicates message acceptance.
 */
static srRetVal psrcSendAckMesg(struct sbChanObject* pChan)
{
	srRetVal iRet;
	sbMesgObj *pMesg;
	static int i = 0;

	sbChanCHECKVALIDOBJECT(pChan);
	
	pMesg = sbMesgConstruct(NULL, "<ok />");
	iRet = sbMesgSendMesg(pMesg, pChan, "RPY", 0);
	sbMesgDestroy(pMesg);

	return iRet;
}



/* ################################################################# *
 * public members                                                    *
 * ################################################################# */
srRetVal psrcOnChanCreate(struct sbProfObject *pThis, struct sbSessObject* pSess, struct sbChanObject* pChan)
{
	sbProfCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);
	sbChanCHECKVALIDOBJECT(pChan);

	/* There is nothing to do for cooked - all is done via
	 * the formatted messages being exchanged ;-)
	 */
	return SR_RET_OK;
}


/**
 * Process a received <entry> element.
 */
static srRetVal psrcOnMesgRecvDoEntry(sbProfObj *pThis, int* pbAbort, sbSessObj* pSess, sbChanObj* pChan, sbMesgObj *pMesg, sbNVTEObj *pEntry)
{
	srRetVal iRet;
	srSLMGObj *pSLMG;
	char *pszRemHostIP;

	sbProfCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);
	sbMesgCHECKVALIDOBJECT(pMesg);
	sbNVTECHECKVALIDOBJECT(pEntry);
	assert(pbAbort != NULL);

    /* We got the message */
	if((iRet = srSLMGConstruct(&pSLMG)) != SR_RET_OK)
	{
		return iRet;
	}

	pSLMG->iSource = srSLMG_Source_BEEPCOOKED;

	if((iRet = srSLMGSetRawMsg(pSLMG, pEntry->pszValue, FALSE)) != SR_RET_OK)
	{
		srSLMGDestroy(pSLMG);
		return iRet;
	}

	if((iRet = sbSockGetRemoteHostIP(pSess->pSock, &pszRemHostIP)) != SR_RET_OK)
	{
		srSLMGDestroy(pSLMG);
		return iRet;
	}
	if((iRet = srSLMGSetRemoteHostIP(pSLMG, pszRemHostIP, FALSE)) != SR_RET_OK)
	{
		srSLMGDestroy(pSLMG);
		free(pszRemHostIP);
		return iRet;
	}
	if((iRet = srSLMGParseMesg(pSLMG)) != SR_RET_OK)
	{
		srSLMGDestroy(pSLMG);
		free(pszRemHostIP);
		return iRet;
	}
	pThis->pAPI->OnSyslogMessageRcvd(pThis->pAPI, pSLMG);
	free(pszRemHostIP);
	srSLMGDestroy(pSLMG);

	return SR_RET_OK;
}



/**
 * Process the incoming BEEP Message and call the upper-layer event 
 * handler. Most importantly, this method must parse and extract the
 * syslog messages inside the BEEP Message.
 *
 * If there are any error conditions, THIS method must notify
 * the remote peer. The reason is that this method has a much
 * better knowledge to provide a meaningful error message than
 * its upper layer caller.
 */
static srRetVal psrcOnMesgRecvCallAPI(sbProfObj *pThis, int* pbAbort, sbSessObj* pSess, sbChanObj* pChan, sbMesgObj *pMesg)
{
	srRetVal iRet = SR_RET_OK;
	sbNVTRObj *pMsgXML;
	sbNVTEObj *pEntry;
	char szErrMsg[1024];

	sbProfCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);
	sbMesgCHECKVALIDOBJECT(pMesg);
	assert(pbAbort != NULL);

	pMsgXML = sbNVTRConstruct();
	if((iRet = sbNVTRParseXML(pMsgXML , pMesg->szActualPayload)) != SR_RET_OK)
	{	/* we need to notify the peer that something went wrong */
		SNPRINTF(szErrMsg, sizeof(szErrMsg), "Error %d parsing XML - is it malformed?", iRet);
		sbChanSendErrResponse(pChan, 550, szErrMsg);
		return iRet;
	}

#	if DEBUGLEVEL > 1
		sbNVTDebugPrintTree(pMsgXML, 0);
#	endif

	/* In the next block, we do not return() if we detect an error
	 * but we set iRet and run down below. This makes cleanup easier.
	 */
	if((pEntry = sbNVTRHasElement(pMsgXML , "entry", TRUE)) != NULL)
	{
		iRet = psrcOnMesgRecvDoEntry(pThis, pbAbort, pSess, pChan, pMesg, pEntry);
	}
	else if((pEntry = sbNVTRHasElement(pMsgXML , "path", TRUE)) != NULL)
	{
		printf("Path, Msg: %s\n", pMesg->szActualPayload);
		iRet = SR_RET_OK;
	}
	else if((pEntry = sbNVTRHasElement(pMsgXML , "iam", TRUE)) != NULL)
	{
		printf("iam, Msg: %s\n", pMesg->szActualPayload);
		iRet = SR_RET_OK;
	}
	else
	{	/* this is an invalid XML stream, so complain back */
		sbChanSendErrResponse(pChan, 500, "Invalid XML for this profile - <entry>, <iam> or <path> expected but not found - maybe malformed XML.");
		iRet = SR_RET_XML_MALFORMED;
		*pbAbort = TRUE;
	}

	sbNVTRDestroy(pMsgXML);

	return iRet;
}


srRetVal psrcOnMesgRecv(sbProfObj *pThis, int* pbAbort, sbSessObj* pSess, sbChanObj* pChan, sbMesgObj *pMesg)
{
	srRetVal iRet;

	sbProfCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);
	sbChanCHECKVALIDOBJECT(pChan);
	sbMesgCHECKVALIDOBJECT(pMesg);
	assert(pbAbort != NULL);

	switch(pMesg->idHdr)
	{
	case BEEPHDR_MSG:
		if(pThis->pAPI->OnSyslogMessageRcvd != NULL)
		{	/* Call handler - as of RFC 3195, we may potentially
			 * have more than one message inside a single packet.
			 * As such, we now need to disassmble the packet and
			 * pass the messages individually.
			 */
			if((iRet = psrcOnMesgRecvCallAPI(pThis, pbAbort, pSess, pChan, pMesg)) != SR_RET_OK)
				return iRet;
		}
		/* See http://lists.beepcore.org/pipermail/beepwg/2003-September/001540.html
		 * for what this means, below. Here is an excerpt, should the URL disappear...

		 		There are two advancing "message ids" in BEEP. One is the seqno, the
				other is the msgno. 

				Msgno is used at the application layer to correlate requests and
				responses. A peer may issue many MSGs without necessarily receiving
				RPYs, but the remote peer MUST send RPYs in the same sequence that it
				received MSGs for. This is detailled under  "Asynchrony" in RFC3080,
				2.6. 

				Msgno is NOT intended to be used for flow control. Hence, there is no
				and must no window be defined for it.

				In contrast, seqno is the one used for actual flow control. Windowing is
				defined in RFC3081 and carried out via SEQ frames.

				So if I implement a BEEP-based server AND I need to limit the amount of
				data that an application sends to me, I do not do this via MSG/RPY but
				via SEQ only. So it is vital that a robust server design includes some
				type of communication of the upper app layers with the lower 3081 layers
				so that the later will receive feedback from the upper layers when it is
				time to stop sending SEQs. The good thing about this is that flow
				control follows a single paradigm, no matter if MSG/RPY or MSG/ANS
				semantics are used.

		 */
		if((iRet = psrcSendAckMesg(pChan)) != SR_RET_OK)
			return iRet;

		/* quick & dirty to get it this week done - send SEQ ;) */
		sbChanSendSEQ(pChan, pMesg->uNxtSeqno, 0);
		break;
	default:
		sbChanSendErrResponse(pChan, 550, "Invalid MSG type. Only MSG messages accepted by this profile - see RFC 3195/COOKED.");
		return SR_RET_INAPROPRIATE_HDRCMD;
	}

	return SR_RET_OK;
}

