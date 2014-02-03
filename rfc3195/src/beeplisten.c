/*! \file beeplisten.c
 *  \brief Implementation of the (BEEP) listener Object.
 *
 * Actually, this module implements all the listeners. The initial
 * release did only care about BEEP, but starting with 0.6.0 additional
 * listeners are supported. In 0.6.0 at least a standard UDP listener
 * has been added. Other listeners (eventually not implemented by the
 * time you read this) are the simple syslog over TCP (aka "SELP") and
 * the local /var/log "listener" on *nix.
 *
 * Please see \ref architecture.c for more details.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-13
 *			File initially created.
 * \date    2003-09-26
 *			Changed the logical meaning of this module. It is no longer
 *          the BEEP listener, only. It is now *the* listener for
 *          all supported protocols. This allows us to keep all
 *          listeners on a single thread.
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
#include <ctype.h>
#include "settings.h"
#include "liblogging.h"
#include "sockets.h"
#include "beepsession.h"
#include "beepchannel.h"
#include "beepframe.h"
#include "beeplisten.h"
#include "namevaluetree.h"
#include "stringbuf.h"
#include "syslogmessage.h"


/* ################################################################# *
 * private members                                                   *
 * ################################################################# */
#if FEATURE_UNIX_DOMAIN_SOCKETS == 1
/**
 * Process an incoming Unix Domain Socket message.
 */
static srRetVal sbLstnRecvUXDOMSOCK(sbLstnObj *pThis)
{
	srRetVal iRet;
	int iLenBuf;	
	int iLenRcvd;
	char *pszFromHost;
	srSLMGObj *pSLMG;
	char szMsgBuf[BEEPFRAMEMAX];

	sbLstnCHECKVALIDOBJECT(pThis);

	iLenBuf = sizeof(szMsgBuf) / sizeof(char);
	if((iLenRcvd  = sbSockReceive(pThis->pSockUXDOMSOCKListening, szMsgBuf, iLenBuf)) < 1)
		return SR_RET_OK /* for now, this seems to be good enough... */;

	/* We got the message - pass it to the "API" layer */

	if((iRet = srSLMGConstruct(&pSLMG)) != SR_RET_OK)
	{
		return iRet;
	}

	pSLMG->iSource = srSLMG_Source_UX_DFLT_DOMSOCK;

	if((iRet = srSLMGSetRawMsg(pSLMG, szMsgBuf, TRUE)) != SR_RET_OK)
	{
		srSLMGDestroy(pSLMG);
		return iRet;
	}

	if((iRet = sbSock_gethostname((char**) &pszFromHost)) != SR_RET_OK)
	{
		srSLMGDestroy(pSLMG);
		return iRet;
	}

	if((iRet = srSLMGSetRemoteHostIP(pSLMG, pszFromHost, FALSE)) != SR_RET_OK)
	{
		srSLMGDestroy(pSLMG);
		free(pszFromHost);
		return iRet;
	}

	if((iRet = srSLMGParseMesg(pSLMG)) != SR_RET_OK)
	{
		srSLMGDestroy(pSLMG);
		return iRet;
	}
	pThis->pAPI->OnSyslogMessageRcvd(pThis->pAPI, pSLMG);
	srSLMGDestroy(pSLMG);

	return SR_RET_OK;
}
#endif  /* FEATURE_UNIX_DOMAIN_SOCKETS */

#if FEATURE_UDP == 1
/**
 * Process an incoming UDP message.
 *
 * Please note that in UDP, we do not have any profiles. As such, we can
 * call the API handler immediately (we know what to do in our processing).
 */
static srRetVal sbLstnRecvUDP(sbLstnObj *pThis)
{
	srRetVal iRet;
	int iLenBuf;	
	char *pszFromHost;
	srSLMGObj *pSLMG;
	char szMsgBuf[BEEPFRAMEMAX];

	sbLstnCHECKVALIDOBJECT(pThis);

	iLenBuf = sizeof(szMsgBuf) / sizeof(char);
	if((iRet = sbSockRecvFrom(pThis->pSockUDPListening, szMsgBuf, &iLenBuf,
		                       &pszFromHost)) != SR_RET_OK)
		return iRet;

	/* We got the message - pass it to the "API" layer */

	if((iRet = srSLMGConstruct(&pSLMG)) != SR_RET_OK)
	{
		return iRet;
	}

	pSLMG->iSource = srSLMG_Source_UDP;

	if((iRet = srSLMGSetRawMsg(pSLMG, szMsgBuf, TRUE)) != SR_RET_OK)
	{
		srSLMGDestroy(pSLMG);
		return iRet;
	}

	if((iRet = srSLMGSetRemoteHostIP(pSLMG, pszFromHost, FALSE)) != SR_RET_OK)
	{
		srSLMGDestroy(pSLMG);
		free(pszFromHost);
		return iRet;
	}
	if((iRet = srSLMGParseMesg(pSLMG)) != SR_RET_OK)
	{
		srSLMGDestroy(pSLMG);
		free(pszFromHost);
		return iRet;
	}
	pThis->pAPI->OnSyslogMessageRcvd(pThis->pAPI, pSLMG);
	srSLMGDestroy(pSLMG);

	free(pszFromHost);

	return SR_RET_OK;
}
#endif  /* FEATURE_UDP */


/**
 * Lowest level event handler when a new frame arrived.
 * This handler must first verify if it is a good packet
 * (checks like seqno sequence and thus) and if so, find the
 * profile that it should send data to. SEQ frames are directly
 * processed and channel 0 protocol is mapped to fixed handlers
 * in this object itself.
 *
 * Please note: the message object created here is passed on
 *              to the upper layers. However, these upper layers
 *              MUST NOT destroy the Mesg Object - this will be 
 *              done once this method finishes!
 *
 * \param pSess Pointer to the current session object.
 * \param pFram Pointer to the just received frame. 
 */
srRetVal sbLstnOnFramRcvd(sbLstnObj* pThis, int *pbAbort, sbSessObj* pSess, sbFramObj *pFram)
{
	srRetVal iRet;
	sbProfObj *pProf;
	sbChanObj *pChan;
	sbMesgObj *pMesg;

	sbLstnCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);
	sbFramCHECKVALIDOBJECT(pFram);
	assert(pbAbort != NULL);

	/* As data arrived, the "need data" indicator can be reset. We do not
	 * care if the right data arrived - if it didn't, its not a big deal,
	 * at most we do one unncessary call to the send method. This
	 * will now enable sending data if it was disabled on the session.
	 */
	pSess->bNeedData = FALSE;

	/* find the associated channel & profile */
	if((pChan = sbSessRetrChanObj(pSess, pFram->uChannel)) == NULL)
	{
		sbFramDestroy(pFram);	/* we now no longer need this */
		return SR_RET_CHAN_DOESNT_EXIST;
	}

	/* we now need to assemble a message out of the frame */
	pMesg = sbMesgConstrFromFrame(pFram);
	sbFramDestroy(pFram);	/* we now no longer need this */
	if(pMesg == NULL)
		return SR_RET_ERR;

	/* pass control to upper layer */
	pProf = pChan->pProf;
	sbProfCHECKVALIDOBJECT(pProf);

	/* OK, we got the profile, so let's call the event handler */
	if(pProf->OnMesgRecv == NULL)
	{
		sbChanSendErrResponse(pChan, 451, "local profile error: OnMesgRecv handler is missing - contact software vendor");
		return SR_RET_ERR_EVENT_HANDLER_MISSING;
	}

	if((iRet = (pProf->OnMesgRecv)(pProf, pbAbort, pSess, pChan, pMesg)) != SR_RET_OK)
		return iRet;

	sbMesgDestroy(pMesg);

	return SR_RET_OK;
}


/**
 * This method build a frame by processing one character at a
 * time. It operates on a continuous stream of data. It is 
 * a state machine.
 * 
 * As soon as a full frame is detected, the frame handler is called 
 * which then should pass the data to his upper layer - or whatever
 * he intends to do with it. This is not the business of this
 * method. For each new frame, new memory is allocated, so
 * the frame handler (or its upper layers) are responsible
 * for deleting the frame.
 *
 * This method calls itself recursively if it needed to peek
 * at a character and now needs to do the actual processing.
 */
srRetVal sbLstnBuildFrame(sbLstnObj* pThis, sbSessObj* pSess, char c, int *pbAbort)
{
	srRetVal iRet;
	sbFramObj *pFram;
	char* pBuf;

	sbLstnCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);
	assert(pbAbort != NULL);

	if((pFram = pSess->pRecvFrame) == NULL)
	{	/* OK, we are ready for the next one, so
		 * let's allocate it ;)
		 */
		if((iRet = sbFramConstruct(&pFram)) != SR_RET_OK)
			return iRet;
		pFram->iState = sbFRAMSTATE_WAITING_HDR1;
		pSess->pRecvFrame = pFram;
	}

	switch(pFram->iState)
	{
	case sbFRAMSTATE_WAITING_HDR1:	/* waiting for the first HDR character */
		if((pFram->pStrBuf = sbStrBConstruct()) == NULL)
			return SR_RET_OUT_OF_MEMORY;
		if(c != 'A' && c != 'E' && c != 'M'  && c != 'N' && c != 'R' && c != 'S')
			return SR_RET_INVALID_HDRCMD;
		if((iRet = sbStrBAppendChar(pFram->pStrBuf, c)) != SR_RET_OK)
			return iRet;
		pFram->iState = sbFRAMSTATE_WAITING_HDR2;
		break;
	case sbFRAMSTATE_WAITING_HDR2:	/* waiting for the second HDR character */
		if(c != 'N' && c != 'R' && c != 'S'  && c != 'U' && c != 'P' && c != 'E')
			return SR_RET_INVALID_HDRCMD;
		if((iRet = sbStrBAppendChar(pFram->pStrBuf, c)) != SR_RET_OK)
			return iRet;
		pFram->iState = sbFRAMSTATE_WAITING_HDR3;
		break;
	case sbFRAMSTATE_WAITING_HDR3:	/* waiting for the third HDR character */
		if((iRet = sbStrBAppendChar(pFram->pStrBuf, c)) != SR_RET_OK)
			return iRet;
		pBuf = sbStrBFinish(pFram->pStrBuf);
		pFram->pStrBuf = NULL;
		if((pFram->idHdr = sbFramHdrID(pBuf)) == BEEPHDR_UNKNOWN)
			return SR_RET_INVALID_HDRCMD;
		free(pBuf);
		pFram->iState = sbFRAMSTATE_WAITING_SP_CHAN;
		break;
	case sbFRAMSTATE_WAITING_SP_CHAN:/* waiting for the SP before channo */
		if(c != ' ')
			return SR_RET_INVALID_WAITING_SP_CHAN;
		pFram->uChannel = 0;
		pFram->iState = sbFRAMSTATE_IN_CHAN;
		break;
	case sbFRAMSTATE_IN_CHAN:		/* reading channo */
		if(isdigit(c))
			pFram->uChannel = pFram->uChannel * 10 + (c - '0');
		else
		{
			pFram->iState = (pFram->idHdr == BEEPHDR_SEQ) ? sbFRAMSTATE_WAITING_SP_ACKNO : sbFRAMSTATE_WAITING_SP_MSGNO;
			return sbLstnBuildFrame(pThis, pSess, c, pbAbort);
		}
		break;
	case sbFRAMSTATE_WAITING_SP_ACKNO:/* now the space before the next header */
		if(c != ' ')
			return SR_RET_INVALID_WAITING_SP_ACKNO;
		pFram->uAckno = 0;
		pFram->iState = sbFRAMSTATE_IN_ACKNO;
		break;
	case sbFRAMSTATE_IN_ACKNO:		/* and the next (char) header */
		if(isdigit(c))
			pFram->uAckno = pFram->uAckno * 10 + (c - '0');
		else
		{
			pFram->iState = sbFRAMSTATE_WAITING_SP_WINDOW;
			return sbLstnBuildFrame(pThis, pSess, c, pbAbort);
		}
		break;
	case sbFRAMSTATE_WAITING_SP_WINDOW:/* now the space before the next header */
		if(c != ' ')
			return SR_RET_INVALID_WAITING_SP_WINDOW;
		pFram->uWindow = 0;
		pFram->iState = sbFRAMSTATE_IN_WINDOW;
		break;
	case sbFRAMSTATE_IN_WINDOW:		/* and the next (numeric) header */
		if(isdigit(c))
			pFram->uWindow = pFram->uWindow * 10 + (c - '0');
		else
		{
			pFram->iState = sbFRAMSTATE_WAITING_HDRCR;
			return sbLstnBuildFrame(pThis, pSess, c, pbAbort);
		}
		break;
	case sbFRAMSTATE_WAITING_SP_MSGNO:/* now the space before the next header */
		if(c != ' ')
			return SR_RET_INVALID_WAITING_SP_MSGNO;
		pFram->uMsgno = 0;
		pFram->iState = sbFRAMSTATE_IN_MSGNO;
		break;
	case sbFRAMSTATE_IN_MSGNO:		/* and the next (numeric) header */
		if(isdigit(c))
			pFram->uMsgno = pFram->uMsgno * 10 + (c - '0');
		else
		{
			pFram->iState = sbFRAMSTATE_WAITING_SP_MORE;
			return sbLstnBuildFrame(pThis, pSess, c, pbAbort);
		}
		break;
	case sbFRAMSTATE_WAITING_SP_MORE:/* now the space before the next header */
		if(c != ' ')
			return SR_RET_INVALID_WAITING_SP_MORE;
		pFram->iState = sbFRAMSTATE_IN_MORE;
		break;
	case sbFRAMSTATE_IN_MORE:		/* and the next (char) header */
		if(c != '.' && c != '*')
			return SR_RET_INVALID_IN_MORE;
		pFram->cMore = c;
		pFram->iState = sbFRAMSTATE_WAITING_SP_SEQNO;
		break;
	case sbFRAMSTATE_WAITING_SP_SEQNO:/* now the space before the next header */
		if(c != ' ')
			return SR_RET_INVALID_WAITING_SP_SEQNO;
		pFram->uSeqno = 0;
		pFram->iState = sbFRAMSTATE_IN_SEQNO;
		break;
	case sbFRAMSTATE_IN_SEQNO:		/* and the next (numeric) header */
		if(isdigit(c))
			pFram->uSeqno = pFram->uSeqno * 10 + (c - '0');
		else
		{
			pFram->iState = sbFRAMSTATE_WAITING_SP_SIZE;
			return sbLstnBuildFrame(pThis, pSess, c, pbAbort);
		}
		break;
	case sbFRAMSTATE_WAITING_SP_SIZE:/* now the space before the next header */
		if(c != ' ')
			return SR_RET_INVALID_WAITING_SP_SIZE;
		pFram->uSize = 0;
		pFram->iState = sbFRAMSTATE_IN_SIZE;
		break;
	case sbFRAMSTATE_IN_SIZE:		/* and the next (numeric) header */
		if(isdigit(c))
			pFram->uSize = pFram->uSize * 10 + (c - '0');
		else
		{
			/* We need to guard against oversized frames here.
			* To do it fully right, we would need to check the
			* remaining bytes in the window and compare this against
			* uSize. However, we right now do it somewhat simpler and
			* just compare against the max window size. Later releases
			* can than add the more complex checks. This one here is
			* at least good enough against malicous frames... )
			*/
			/** \todo improve! */
			if(pFram->uSize > BEEPFRAMEMAX)
				return SR_RET_OVERSIZED_FRAME;
			pFram->iState = (pFram->idHdr == BEEPHDR_ANS) ? sbFRAMSTATE_WAITING_SP_ANSNO : sbFRAMSTATE_WAITING_HDRCR;
			return sbLstnBuildFrame(pThis, pSess, c, pbAbort);
		}
		break;
	case sbFRAMSTATE_WAITING_SP_ANSNO:/* now the space before the next header */
		if(c != ' ')
			return SR_RET_INVALID_WAITING_SP_ANSNO;
		pFram->uAnsno = 0;
		pFram->iState = sbFRAMSTATE_IN_ANSNO;
		break;
	case sbFRAMSTATE_IN_ANSNO:		/* and the next (numeric) header */
		if(isdigit(c))
			pFram->uAnsno = pFram->uAnsno * 10 + (c - '0');
		else
		{
			pFram->iState = sbFRAMSTATE_WAITING_HDRCR;
			return sbLstnBuildFrame(pThis, pSess, c, pbAbort);
		}
		break;
	case sbFRAMSTATE_WAITING_HDRCR:	/* awaiting the HDR's CR */
		if(c != 0x0d)
			return SR_RET_INVALID_WAITING_HDRCR;
		pFram->iState = sbFRAMSTATE_WAITING_HDRLF;
		break;
	case sbFRAMSTATE_WAITING_HDRLF:	/* awaiting the HDR's LF */
		if(c != 0x0a)
			return SR_RET_INVALID_WAITING_HDRLF;
		if(pFram->idHdr == BEEPHDR_SEQ)
		{	/* frame is fully received and ready for processing*/
			/* call the profile event! */
			pSess->pRecvFrame = NULL;	/* We are done, so let's move to the next one ;) */
			return sbLstnOnFramRcvd(pThis, pbAbort, pSess, pFram);
		}
		else
		{
			if((pFram->pStrBuf = sbStrBConstruct()) == NULL)
				return SR_RET_OUT_OF_MEMORY;
			pFram->uToReceive = pFram->uSize;
			pFram->iState = (pFram->uToReceive > 0) ? sbFRAMSTATE_IN_PAYLOAD : sbFRAMSTATE_WAITING_END1;
		}
		break;
	case sbFRAMSTATE_IN_PAYLOAD:		/* reading payload area */
		if((iRet = sbStrBAppendChar(pFram->pStrBuf, c)) != SR_RET_OK)
			return iRet;
		if(--(pFram->uToReceive) == 0)
			pFram->iState = sbFRAMSTATE_WAITING_END1;
		break;
	case sbFRAMSTATE_WAITING_END1:	/* waiting for the 1st HDR character */
		/* we first need to save the payload area */
		pFram->szRawBuf = sbStrBFinish(pFram->pStrBuf);
		pFram->pStrBuf = NULL;
		pFram->iFrameLen = pFram->uSize;
		/* now on with "normal" checking */
		if(c != 'E')
			return SR_RET_INVALID_WAITING_END1;
		pFram->iState = sbFRAMSTATE_WAITING_END2;
		break;
	case sbFRAMSTATE_WAITING_END2:	/* waiting for the 2nd HDR character */
		if(c != 'N')
			return SR_RET_INVALID_WAITING_END2;
		pFram->iState = sbFRAMSTATE_WAITING_END3;
		break;
	case sbFRAMSTATE_WAITING_END3:	/* waiting for the 3rd HDR character */
		if(c != 'D')
			return SR_RET_INVALID_WAITING_END3;
		pFram->iState = sbFRAMSTATE_WAITING_END4;
		break;
	case sbFRAMSTATE_WAITING_END4:	/* waiting for the 4th HDR character */
		if(c != 0x0d)
			return SR_RET_INVALID_WAITING_END4;
		pFram->iState = sbFRAMSTATE_WAITING_END5;
		break;
	case sbFRAMSTATE_WAITING_END5:	/* waiting for the 5th HDR character */
		if(c != 0x0a)
			return SR_RET_INVALID_WAITING_END5;
		/* frame is fully received and ready for processing*/
		/* call the profile event! */
		pSess->pRecvFrame = NULL;	/* We are done, so let's move to the next one ;) */
		return sbLstnOnFramRcvd(pThis, pbAbort, pSess, pFram);
		break;
	default:
		return SR_RET_ERR;
	}

	return SR_RET_OK;
}


/**
 * Receive all incoming data that is available.
 * This method reads all available incoming data and processes
 * it. It will call receive state machine to form new frames.
 * That machine, in turn, will fire off Receive events as 
 * full frames are completely received. Please note that it is possible
 * for a single run of this method to receive no, one or more full
 * frames.
 */
srRetVal sbLstnDoIncomingData(sbLstnObj* pThis, sbSessObj* pSess)
{
	int iBytesRcvd;	/**< actual number of bytes received */
	char* pBuf;
	srRetVal iRet;
	int bAbort;		/**< set by lower layer methods if a fatal error
					 ** occured. in this case, the session should be
					 ** aborted and such the full buffer needs not to 
					 ** be processed.
					 **/
	char szRcvBuf[1600]; /**< receive buffer - this could be any size, we 
					      * have selected this size so that an ethernet frame
					      * fits. Feel free to tune. */

	sbLstnCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);

	if((iBytesRcvd = sbSockReceive(pSess->pSock, szRcvBuf, sizeof(szRcvBuf))) == 0)
		return SR_RET_CONNECTION_CLOSED;

	if(iBytesRcvd == SOCKET_ERROR)
		if(pSess->pSock->dwLastError != SBSOCK_EWOULDBLOCK)
			return SR_RET_SOCKET_ERR;

	bAbort = FALSE;
	pBuf = szRcvBuf;
	while(iBytesRcvd--)
		if((iRet = sbLstnBuildFrame(pThis, pSess, *pBuf++, &bAbort)) != SR_RET_OK)
			if(bAbort == TRUE)
				return iRet;

	return SR_RET_OK;
}


/**
 * This method is used to "free" a session. If there is still
 * a valid session pointer in the structure when the tree is
 * deleted, that means something went wrong and we need to get
 * rid of the leftovers. As such, any sessions found are aborted.
 */
static void sbLstnSessFreeLinkedListDummy(sbSessObj *pSess)
{
	int bShouldNeverBeCalled = 0;

	sbSessCHECKVALIDOBJECT(pSess);
	sbSessAbort(pSess);
}


/**
 * Add a session to the list of active sessions.
 */
srRetVal sbSessAddActiveSession(sbLstnObj* pThis, sbSessObj *pSess)
{
	sbNVTEObj* pEntry;

	sbLstnCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);

	if((pEntry = sbNVTAddEntry(pThis->pRootSessions)) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	sbNVTESetUsrPtr(pEntry, pSess, (void*) sbLstnSessFreeLinkedListDummy);
	
	return SR_RET_OK;
}


/**
 * This method is called when a new session is initiated
 * by a client. A new session object will be generated and
 * put into the linked list of existing session objects.
 * Then, a channel 0 greeting will be issued.
 */
static srRetVal sbLstnNewSess(sbLstnObj* pThis)
{
	srRetVal iRet;
	sbSockObj *pNewSock;
	sbSessObj *pSess;

	/* Accept the connection, make room for next clients ;) */
	if((iRet = sbSockAcceptConnection(pThis->pSockListening, &pNewSock) != SR_RET_OK))
		return iRet;

	if((iRet = sbSockSetNonblocking(pNewSock) != SR_RET_OK))
	{
		sbSockExit(pNewSock); /* best we can do */
		return iRet;
	}
	
	/* begin constructing the profile */
	if((iRet = sbSessRemoteOpen(&pSess, pNewSock, pThis->pProfsSupported)) != SR_RET_OK)
	{
		sbSockExit(pNewSock); /* best we can do */
		return iRet;
	}

	/* The channel is ready, now we need to add the profile */

	if((iRet = sbSessAddActiveSession(pThis, pSess)) != SR_RET_OK)
	{
		sbSessDestroy(pSess);
		sbSockExit(pNewSock);
		return iRet;
	}

	return sbSessSendGreeting(pSess, pThis->pProfsSupported);
}

/**
 * Actually send a frame over the socket. If the method would
 * block, we could only send a partial frame.
 *
 * \todo We need to upgrade either this method (or create another one
 * so that it will send multiple frames if multiple frames are
 * in the send queue. Right now, this is done by the select() loop
 * anyhow, but it may be more efficient to do it that way (or it may
 * be not, this needs some more thinking...).
 *
 * IMPORTANT: When sending SEQ frames, we do not 
 * use the "normal" window mechanism. Essentially, these
 * SEQs are acknowledgements, so they are not to be
 * considered as part of the data window. If we would
 * count them, we would stall the process as the peer
 * will abviously never acknowledge acknowledgements... ;)
 */
srRetVal sbLstnSendFram(sbLstnObj* pThis, sbSessObj *pSess)
{
	int iBytes2Send;
	int iBytesSent;
	char* pBuf;
	sbFramObj *pFram;

	sbLstnCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);

	if(pSess->pSendQue->pFirst == NULL)
		return SR_RET_OK;

	pFram = (sbFramObj*) pSess->pSendQue->pFirst->pUsr;
	sbFramCHECKVALIDOBJECT(pFram);

	/* We now need to check if this is a full frame that
	 * we need to begin to send. If so, we must first 
	 * check if we are within the current window size.
	 * If not, we must wait for the next SEQ frame on this
	 * channel and thus can not send immediately, even if
	 * the socket would be ready.
	 */
	if(pFram->iState == sbFRAMSTATE_READY_TO_SEND)
	{	/** \todo We need to see if race conditions can occur
		 * where pChan becomes invalid but is still stored
		 * with the message. It may be a better idea to use
		 * the "normal" channel lookup methods, though they
		 * are definitely slower...
		 * For now, we use an assert and will test this well.
		 */
		sbChanCHECKVALIDOBJECT(pFram->pChan);
		if( (pFram->idHdr != BEEPHDR_SEQ)
		    && ((unsigned) pFram->iFrameLen > pFram->pChan->uTXWinLeft))
		{
			pSess->bNeedData = TRUE;
			return SR_RET_REMAIN_WIN_TOO_SMALL;
		}
	}

	iBytes2Send = pFram->iFrameLen - pFram->uBytesSend;
	assert(iBytes2Send > 0);
	pBuf = pFram->szRawBuf + pFram->uBytesSend;

	iBytesSent = sbSockSend(pSess->pSock, pBuf, iBytes2Send);
    if((iBytesSent > iBytes2Send) || (iBytesSent < 0))
		return SR_RET_SOCKET_ERR;

	pFram->uBytesSend += iBytesSent;

	/* now check if all is send and we can remove the frame */
	if((pFram->uBytesSend == pFram->iFrameLen))
	{
		pFram->iState = sbFRAMSTATE_SENT;
		/** \todo again, referencing the channel object as done below
		 ** is a little dangerous... */
		if(pFram->idHdr != BEEPHDR_SEQ)
			pFram->pChan->uTXWinLeft -= pFram->iFrameLen;
		sbNVTRRemoveFirst(pSess->pSendQue);
	}
	else
		pFram->iState = sbFRAMSTATE_SENDING;

	return SR_RET_OK;
}


/**
 * This is the main server/IO handler. This loop is executed
 * until termination is flagged. This is the ONLY method
 * that is responsible for scheduling IO.
 *
 * The loop has three phases:
 * - check if data is to be sent and, if so, try to send it
 * - wait for socket to become ready for read or write
 * - process pending socket reads
 *
 * We have some OS dependant code in here. The reason is
 * that "original" BSD sockets need to have passed the
 * highest numbered file descriptor. We can not properly
 * emulate this in the socket layer. Even if we could,
 * that would potentially cause a considerable performance
 * hit. Thus, we implement it in this module and pass it
 * on to the lower layer. If we run into a non BSD-sockets
 * OS other than Windows, we might be not so happy with this
 * decision, but lets try it...
 *
 * Please note that all send operations are only done in phase 1.
 */
srRetVal sbLstnServerLoop(sbLstnObj* pThis)
{
	srRetVal iRet;
	srSock_fd_set fdsetRD;	/* for select() */
	srSock_fd_set fdsetWR;	/* for select() */
	sbNVTEObj *pEntrySess;	/* active session from session linked list */
	sbNVTEObj *pEntrySessToDel;
	sbSessObj *pSess;
	sbFramObj *pFram;
#	ifndef SROS_WIN32
	int	iHighestDesc;
#	endif
	int	iReturnSock;

	sbLstnCHECKVALIDOBJECT(pThis);

	while(pThis->bRun == TRUE)
	{
		/* phase 1: send data (if any)
		* We loop through all sessions to see if there is something
		* pending.
		*/
		pEntrySess = NULL;
		while((pEntrySess = sbNVTSearchKeySZ(pThis->pRootSessions, pEntrySess, NULL)) != NULL)
		{
			pSess = ((sbSessObj*) pEntrySess->pUsr);
			if(pSess->pSendQue != NULL)
				if(pSess->pSendQue->pFirst != NULL)	/* ok, quick hack, but this is FAST... */
				{
					pFram = (sbFramObj*) pSess->pSendQue->pFirst->pUsr;
					if(pFram->iState == sbFRAMSTATE_READY_TO_SEND)
						/* we eventually have something to send */
						sbLstnSendFram(pThis, pSess);
				}
		}
		

		/* assemble list of all active sockets & figure out & remove closed ones */
		sbSockFD_ZERO(&fdsetWR);
		sbSockFD_ZERO(&fdsetRD);

		/* add BEEP listener (can not be turned off so far) */
#		ifndef SROS_WIN32
			iHighestDesc = pThis->pSockListening->sock;
#		endif
		sbSockFD_SET(pThis->pSockListening->sock, &fdsetRD);

		/* add UDP listener (if configured) */
		if(pThis->bLstnUDP == TRUE)
		{
#		ifndef SROS_WIN32
			if(pThis->pSockUDPListening->sock > iHighestDesc) 
				iHighestDesc = pThis->pSockUDPListening->sock;
#		endif
		sbSockFD_SET(pThis->pSockUDPListening->sock, &fdsetRD);
		}
	
		/* UNIX Domain Sockets (if configured) */
#		if FEATURE_UNIX_DOMAIN_SOCKETS
		if(pThis->bLstnUXDOMSOCK == TRUE)
		{
			if(pThis->pSockUXDOMSOCKListening->sock > iHighestDesc) 
				iHighestDesc = pThis->pSockUXDOMSOCKListening->sock;
			sbSockFD_SET(pThis->pSockUXDOMSOCKListening->sock, &fdsetRD);
		}
#		endif /* FEATURE_UNIX_DOMAIN_SOCKETS */

		/* add active BEEP sessions */
		pEntrySess = sbNVTSearchKeySZ(pThis->pRootSessions, NULL, NULL);
		while(pEntrySess != NULL)
		{	/* all are added into the read fdset, those with outstanding messages in the
			 * write fdset, too.
			 */
			pSess = ((sbSessObj*) pEntrySess->pUsr);
			if(pSess->iState == sbSESSSTATE_CLOSED)
			{
				/** \todo think a little about the performance implications of the below -
					*        we may think about either doubly linking the list or having
					*        the previous element at hand (e.g. in the linked list object...).
					*/
				pEntrySessToDel = pEntrySess;
				pEntrySess = sbNVTSearchKeySZ(pThis->pRootSessions, pEntrySess, NULL);
				/* debug: printf("shutdown session %8.8x, pUsr %8.8x\n", pEntrySessToDel, pSess); */
				sbNVTRRemovEntryWithpUsr(pThis->pRootSessions, pSess);
			}
			else
			{
				sbSockFD_SET(pSess->pSock->sock, &fdsetRD);
#				ifndef SROS_WIN32
					/* This also takes care of the write descriptor, as
					 * we do this anyway (it is the same descriptor!).
					 */
					if(iHighestDesc < pSess->pSock->sock)
						iHighestDesc = pSess->pSock->sock;
#				endif
				if(pSess->pSendQue->pFirst != NULL)
					sbSockFD_SET(pSess->pSock->sock, &fdsetWR);
				pEntrySess = sbNVTSearchKeySZ(pThis->pRootSessions, pEntrySess, NULL);
			}
		}
		
		/* we are ready now and can wait on the sockets */
#		ifdef SROS_WIN32
			iReturnSock = sbSockSelectMulti(&fdsetRD, &fdsetWR, 10, 0);
#		else
			iReturnSock = sbSockSelectMulti(&fdsetRD, &fdsetWR, 10, 0, iHighestDesc);
#		endif

		if(iReturnSock == -1)
			continue;

		/* First let's see if we have any incoming UDP data (if UDP is configured). 
		 * Please note: the ORDER of conditions is vitally important!
		 */
		if((pThis->bLstnUDP == TRUE) && sbSockFD_ISSET(pThis->pSockUDPListening->sock, &fdsetRD))
		{
			if((iRet = sbLstnRecvUDP(pThis)) != SR_RET_OK)
				printf("UDP error %d!\n", iRet);
		}

		/* now check UNIX Domain Sockets (if configured) */
#		if FEATURE_UNIX_DOMAIN_SOCKETS
		if(pThis->bLstnUXDOMSOCK == TRUE)
		{
			if(sbSockFD_ISSET(pThis->pSockUXDOMSOCKListening->sock, &fdsetRD))
			{
				if((iRet = sbLstnRecvUXDOMSOCK(pThis)) != SR_RET_OK)
					printf("UX DOM SOCK error %d!\n", iRet);
			}
		}
#		endif /* FEATURE_UNIX_DOMAIN_SOCKETS */

		/* Let's see if we have any new BEEP connection requests. There
		 * is only very limited space in the connection buffers, so these
		 * should be served ASAP.
		 */
		if(sbSockFD_ISSET(pThis->pSockListening->sock, &fdsetRD))
		{
			if((iRet = sbLstnNewSess(pThis)) != SR_RET_OK) {
				/* empty by intention */;
				/** \todo log this once we have a logging subsystem */
				/* let me elaborate: if we broke the loop in this case, we
				 * would have won nothing - we would have shut down our server.
				 * This is definitely not what we would like to see just because
				 * a single accept fails (would be a nice way to force DoS ;)).
				 * So we continue processing. If it was an unrecoverable error
				 * (e.g. out of memory), other functions may also fail, but why
				 * care about it before it happens? After all, if some sessions
				 * are drop, memory becomes available and so the server could
				 * continue to operate... (at least in this sample case).
				 */
			}
		}

		/* We then go through all sessions and check which ones
		 * need attention.
		 */
		pEntrySess = sbNVTSearchKeySZ(pThis->pRootSessions, NULL, NULL);
		while(pEntrySess != NULL)
		{
			pSess = ((sbSessObj*) pEntrySess->pUsr);
			/* check read first... */
			if(sbSockFD_ISSET(pSess->pSock->sock, &fdsetRD))
			{	/* OK, we now have received data. We will pass whatever we received off
				 * to the receive state machine. It must handle it. */
				iRet = sbLstnDoIncomingData(pThis, pSess);
				/* there are some error codes which are not actually errors, but warnings. If they
				 * occur, we should NOT shut down the session as whole. They have taken care of their
				 * respective reaction themselfs (hopefully ;)).
				 */
				if(   iRet != SR_RET_OK 
				   && iRet != SR_RET_ERR_EVENT_HANDLER_MISSING)
				{	/* close this connection */
					/* debug: printf("Error %d - closing session.\n", iRet); */
					/* read next entry as we are going to destroy the current one */
					pEntrySess = sbNVTSearchKeySZ(pThis->pRootSessions, pEntrySess, NULL);
					/** \todo think a little about the performance implications of the below -
					 *        we may think about either doubly linking the list or having
					 *        the previous element at hand (e.g. in the linked list object...).
					 */
					sbNVTRRemovEntryWithpUsr(pThis->pRootSessions, pSess);
					continue;
				}
			}

			/* ... then write */
			if(pSess != NULL && sbSockFD_ISSET(pSess->pSock->sock, &fdsetWR))
			{
				sbLstnSendFram(pThis, pSess);
				/** \todo check & act on return value!!!! (close session? most probably...) */
			}
		/* Read next entry. Please note that this was already done if we needed to
		 * destroy a session (see above). In that case, however, a "continue" is used and
		 * control won't reach this point here.
		 */
		pEntrySess = sbNVTSearchKeySZ(pThis->pRootSessions, pEntrySess, NULL);
		}
	}
	return SR_RET_OK;
}


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */

sbLstnObj* sbLstnConstruct(void)
{
	sbLstnObj* pThis;

	if((pThis = calloc(1, sizeof(sbLstnObj))) == NULL)
		return NULL;

	if((pThis->pProfsSupported = sbNVTRConstruct()) == NULL)
	{
		SRFREEOBJ(pThis);
		return NULL;
	}

	pThis->OID = OIDsbLstn;
	pThis->pRootSessions = NULL;
	pThis->pSockListening = NULL;
	pThis->szListenAddr = NULL;
	pThis->uListenPort = 601; /* IANA default for RFC 3195 */
	pThis->bLstnBEEP = TRUE;
	pThis->pAPI = NULL;

#	if FEATURE_UNIX_DOMAIN_SOCKETS == 1
	pThis->bLstnUXDOMSOCK = TRUE;
	pThis->pSockUXDOMSOCKListening = NULL;
	pThis->pSockName = NULL;
#	endif
#	if FEATURE_UDP == 1
	pThis->bLstnUDP = FALSE;
	pThis->uUDPLstnPort = 0;
	pThis->pSockUDPListening = NULL;
#	endif

	if((pThis->pRootSessions = sbNVTRConstruct()) == NULL)
	{
		SRFREEOBJ(pThis);
		return NULL;
	}

	return pThis;
}


void sbLstnDestroy(sbLstnObj* pThis)
{
	sbLstnCHECKVALIDOBJECT(pThis);
	
	if(pThis->pRootSessions != NULL)
		sbNVTRDestroy(pThis->pRootSessions);

	if(pThis->pProfsSupported != NULL)
		sbNVTRDestroy(pThis->pProfsSupported);

	if(pThis->pSockListening != NULL)
		sbSockExit(pThis->pSockListening);

#	if FEATURE_UDP == 1
	if(pThis->pSockUDPListening != NULL)
		sbSockExit(pThis->pSockUDPListening);
#	endif

#	if FEATURE_UNIX_DOMAIN_SOCKETS == 1
	if(pThis->pSockUXDOMSOCKListening != NULL)
		sbSockExit(pThis->pSockUXDOMSOCKListening);
#	endif

	SRFREEOBJ(pThis);
}


srRetVal sbLstnInit(sbLstnObj* pThis)
{
	srRetVal iRet;

	sbLstnCHECKVALIDOBJECT(pThis);

	/* Init BEEP */
	if(pThis->bLstnBEEP == TRUE)
	{
		if((pThis->pSockListening = sbSockInitListenSock(&iRet, SOCK_STREAM, pThis->szListenAddr, pThis->uListenPort)) == NULL)
		{	
			sbLstnDestroy(pThis);
			return iRet; 
		}
	}

	/* Init UDP */
#	if FEATURE_UDP == 1
	if(pThis->bLstnUDP == TRUE)
	{
		if(pThis->uUDPLstnPort == 0)
			/* if the port is not set, we use the IANA assigned port. OK, we
			 * could also do a services lookup, but as of now we prefer to have
			 * the caller provide us correct information. But this may be
			 * reconsidered later. 2003-10-02 RGerhards
			 */
			pThis->uUDPLstnPort = 514; 
		printf("port: %d\n", pThis->uUDPLstnPort);
		if((pThis->pSockUDPListening = sbSockInitListenSock(&iRet, SOCK_DGRAM, pThis->szListenAddr, pThis->uUDPLstnPort)) == NULL)
		{	
			sbLstnDestroy(pThis);
			return iRet; 
		}
	}
#	endif /* FEATURE_UDP */

#	if FEATURE_UNIX_DOMAIN_SOCKETS == 1
	/* Unix Domain Sockets */
	if(pThis->bLstnUXDOMSOCK == TRUE)
	{
		char *pActualSockName;
		pActualSockName = (pThis->pSockName == NULL) ? "/dev/log" : pThis->pSockName;
		printf("listeing to %s (config was %s)\n", pActualSockName, pThis->pSockName);
		if((iRet = sbSock_InitUXDOMSOCK(&(pThis->pSockUXDOMSOCKListening), pActualSockName, /*SOCK_DGRAM*/ SOCK_STREAM)) != SR_RET_OK)
		{	
			sbLstnDestroy(pThis);
			return iRet;
		}
	}
#	endif  /* FEATURE_UNIX_DOMAIN_SOCKETS */

	return(SR_RET_OK);
}


srRetVal sbLstnRun(sbLstnObj* pThis)
{
	srRetVal iRet;

	sbLstnCHECKVALIDOBJECT(pThis);

	pThis->bRun = TRUE;
	/* start listening */

	/* BEEP */
	if(pThis->bLstnBEEP == TRUE)
	{
		if((iRet = sbSockListen(pThis->pSockListening) != SR_RET_OK))
			return iRet;

		if((iRet = sbSockSetNonblocking(pThis->pSockListening) != SR_RET_OK))
		{
			sbSockExit(pThis->pSockListening); /* best we can do */
			return iRet;
		}
	}

#	if FEATURE_UDP == 1
	/* UDP 
	 * In this case, we just need to make it asynchronous
	 */
	if(pThis->bLstnUDP == TRUE)
		if((iRet = sbSockSetNonblocking(pThis->pSockUDPListening) != SR_RET_OK))
		{
			sbSockExit(pThis->pSockUDPListening); /* best we can do */
			return iRet;
		}
#	endif

#	if FEATURE_UNIX_DOMAIN_SOCKETS == 1
	/* Unix Domain Sockets */
	if(pThis->bLstnUXDOMSOCK == TRUE)
		if((iRet = sbSockSetNonblocking(pThis->pSockUXDOMSOCKListening) != SR_RET_OK))
		{
			sbSockExit(pThis->pSockUXDOMSOCKListening); /* best we can do */
			return iRet;
		}
#	endif  /* FEATURE_UNIX_DOMAIN_SOCKETS */
	
	/* done init - now run the server */
	if((iRet = sbLstnServerLoop(pThis)) != SR_RET_OK)
	{
		sbSockExit(pThis->pSockListening); /* best we can do */
		return iRet;
	}

	/* shutdown listening sockets */
	/** \todo think of a smart way to handle iRet down here...  For
	 ** now, we just make sure BEEP is closed last, as this has the
	 ** highest potential for errors.
	 **/
	#	if FEATURE_UDP == 1
	/* UDP */
	if(pThis->bLstnUDP == TRUE)
	{
		iRet = sbSockExit(pThis->pSockUDPListening);
		pThis->pSockUDPListening = NULL;
	}
#	endif

#	if FEATURE_UNIX_DOMAIN_SOCKETS == 1
	/* Unix Domain Sockets */
	if(pThis->bLstnUXDOMSOCK == TRUE)
	{
		iRet = sbSockExit(pThis->pSockUXDOMSOCKListening);
		pThis->pSockUXDOMSOCKListening = NULL;
	}
#	endif  /* FEATURE_UNIX_DOMAIN_SOCKETS */
	
	/* BEEP */
	if(pThis->bLstnBEEP == TRUE)
	{
		iRet = sbSockExit(pThis->pSockListening);
		pThis->pSockListening = NULL;
	}

	return iRet;
}


srRetVal sbLstnExit(sbLstnObj *pThis)
{
	sbLstnCHECKVALIDOBJECT(pThis);
	sbLstnDestroy(pThis);
	return SR_RET_OK;
}


/**
 * Method the free the profile object. This is a helper
 * so that the sbNVTESetUsrPtr() interface can be used.
 * It will actually be called when the listener is 
 * destroyed.
 */
static void sbLstnFreeProf(void *pUsr)
{
	sbProfDestroy((sbProfObj*) pUsr);
}


srRetVal sbLstnAddProfile(sbLstnObj *pThis, sbProfObj *pProf)
{
	sbNVTEObj *pEntry;

	sbLstnCHECKVALIDOBJECT(pThis);
	sbProfCHECKVALIDOBJECT(pProf);
	sbNVTRCHECKVALIDOBJECT(pThis->pProfsSupported);

	if((pEntry = sbNVTAddEntry(pThis->pProfsSupported)) == NULL)
		return SR_RET_OUT_OF_MEMORY;
	sbNVTESetKeySZ(pEntry, pProf->pszProfileURI, TRUE);
	sbNVTESetUsrPtr(pEntry, pProf, sbLstnFreeProf);
	
	return SR_RET_OK;
}
