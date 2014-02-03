/*! \file lstnprof-3195raw.c
 *  \brief Implementation of the listener profile for RFC 3195 raw.
 *
 * The prefix for this "object" is psrr which stands for
*  *P*rofile *S*yslog *R*eliable *Raw*.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-22
 *          coding begun.
 *
 * \date    2003-09-16
 *          Added a check so that failures sending SEQs now do not
 *          go undetected.
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
#include "lstnprof-3195raw.h"
#include "stringbuf.h"
#include "syslogmessage.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */



/* ################################################################# *
 * public members                                                    *
 * ################################################################# */

srRetVal psrrOnChanCreate(struct sbProfObject *pThis, struct sbSessObject* pSess, struct sbChanObject* pChan)
{
	srRetVal iRet;
	sbMesgObj *pMesg;

	sbProfCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);
	sbChanCHECKVALIDOBJECT(pChan);

	pMesg = sbMesgConstruct(NULL, "liblogging (http://www.monitorware.com/liblogging/)  - go ahead.\r\n");
	iRet = sbMesgSendMesg(pMesg, pChan, "MSG", 0);
	sbMesgDestroy(pMesg);

	return iRet;
}


/**
 * Process the incoming BEEP Message and call the upper-layer event 
 * handler. Most importantly, this method must extract the (potentially
 * multiple) syslog messages inside the BEEP Message.
 */
static srRetVal psrrOnMesgRecvCallAPI(sbProfObj *pThis, int* pbAbort, sbSessObj* pSess, sbMesgObj *pMesg)
{
	srRetVal iRet;
	sbStrBObj *pStrBuf;
	int bFoundCRLF; /* 0 = nothing found, 1 = CR FOUND, 2 = CRLF found */
	char *pBuf;
	char *pszMsg;
	char *pszRemHostIP;
	srSLMGObj *pSLMG;

	sbProfCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);
	sbMesgCHECKVALIDOBJECT(pMesg);
	assert(pbAbort != NULL);

	pBuf = pMesg->szActualPayload;
	while(*pBuf)
	{
		if((pStrBuf = sbStrBConstruct()) == NULL)
		{
			*pbAbort = TRUE;
			return SR_RET_OUT_OF_MEMORY;
		}
		bFoundCRLF = 0;
		while(*pBuf && (bFoundCRLF != 2))
		{	/* extract a single message */
			if(*pBuf == '\r')
				bFoundCRLF = 1;
			else if(*pBuf == '\n' && bFoundCRLF == 1)
				bFoundCRLF = 2;
			else
			{
				bFoundCRLF = 0;
				if((iRet = sbStrBAppendChar(pStrBuf, *pBuf)) != SR_RET_OK)
				{
					*pbAbort = TRUE;
					return iRet;
				}
			}
			pBuf++;
		}
		/* We got the message */
		pszMsg = sbStrBFinish(pStrBuf);

		if((iRet = srSLMGConstruct(&pSLMG)) != SR_RET_OK)
		{
			free(pszMsg);
			return iRet;
		}

		pSLMG->iSource = srSLMG_Source_BEEPRAW;

		if((iRet = srSLMGSetRawMsg(pSLMG, pszMsg, FALSE)) != SR_RET_OK)
		{
			srSLMGDestroy(pSLMG);
			free(pszMsg);
			return iRet;
		}

		if((iRet = sbSockGetRemoteHostIP(pSess->pSock, &pszRemHostIP)) != SR_RET_OK)
		{
			srSLMGDestroy(pSLMG);
			free(pszMsg);
			return iRet;
		}
		if((iRet = srSLMGSetRemoteHostIP(pSLMG, pszRemHostIP, FALSE)) != SR_RET_OK)
		{
			srSLMGDestroy(pSLMG);
			free(pszRemHostIP);
			free(pszMsg);
			return iRet;
		}
		if((iRet = srSLMGParseMesg(pSLMG)) != SR_RET_OK)
		{
			srSLMGDestroy(pSLMG);
			free(pszRemHostIP);
			free(pszMsg);
			return iRet;
		}
		pThis->pAPI->OnSyslogMessageRcvd(pThis->pAPI, pSLMG);
		free(pszMsg);
		free(pszRemHostIP);
		srSLMGDestroy(pSLMG);
	}
	return SR_RET_OK;
}


srRetVal psrrOnMesgRecv(sbProfObj *pThis, int* pbAbort, sbSessObj* pSess, sbChanObj* pChan, sbMesgObj *pMesg)
{
	srRetVal iRet;

	sbProfCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);
	sbChanCHECKVALIDOBJECT(pChan);
	sbMesgCHECKVALIDOBJECT(pMesg);
	assert(pbAbort != NULL);

	switch(pMesg->idHdr)
	{
	case BEEPHDR_ANS:
		if(pThis->pAPI->OnSyslogMessageRcvd != NULL)
		{	/* Call handler - as of RFC 3195, we may potentially
			 * have more than one message inside a single packet.
			 * As such, we now need to disassmble the packet and
			 * pass the messages individually.
			 */
			if((iRet = psrrOnMesgRecvCallAPI(pThis, pbAbort, pSess, pMesg)) != SR_RET_OK)
				return iRet;
		}
		/* quick & dirty to get it this week done - send SEQ ;) 
		 * 2003-09-16/RGerhards: actually not as quick and dirty as I initially
		 * thought. I think there is a chance this will stay for quite some while.
		 * Let's see how it evolves...
		 */
		if((iRet = sbChanSendSEQ(pChan, pMesg->uNxtSeqno, 0)) != SR_RET_OK)
			return iRet;
		break;
	case BEEPHDR_NUL:
		sbChanSetAwaitingClose(pChan);
		break;
	default:
		return SR_RET_INAPROPRIATE_HDRCMD;
	}

	return SR_RET_OK;
}
