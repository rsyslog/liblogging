/*! \file beepsession-lstn.c
 *  \brief Implementation of listenere part of the BEEP Session Object.
 *
 * This file implements the listenere part of the BEEP sesion object.
 * logically, it belongs to beepsession.c. The code is included in its
 * own .c file to save some space when linking a pure client app. With
 * its own .c file, the comiled code will not be linked to the application,
 * which it otherwise would be.
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
#include "sockets.h"
#include "beepsession.h"
#include "beepchannel.h"
#include "beepframe.h"
#include "namevaluetree.h"


/* ################################################################# *
 * private members                                                   *
 * ################################################################# */


/**
 * This method is used to "free" a frame from the send queue. This method
 * will be called when an entry is removed from the send queue.
 */
static void sbSessLstnLinkedListFreeFram(sbFramObj *pFram)
{
	sbFramCHECKVALIDOBJECT(pFram);
	sbFramDestroy(pFram);
}


/**
 * Send frame handler for the listener I/O model. It enques the
 * frame instead of actually sending it.
 *
 * \todo It may have some good performance impact, if we tried to
 * send the data initially if there is no other send operation
 * pending. That way, we would avoid en- and de-queueing the frame.
 * But we would need to pay attention to the window!
 * For now, we take the simpler approach - we can always performance
 * tune once it works ;)
 */
static srRetVal sbSessLstnSendFram(sbSessObj *pThis, sbFramObj *pFram, sbChanObj *pChan)
{
	srRetVal iRet;
	sbNVTEObj *pEntry;

	sbSessCHECKVALIDOBJECT(pThis);
	sbFramCHECKVALIDOBJECT(pFram);
	sbChanCHECKVALIDOBJECT(pChan);
	assert(pThis->pSendQue != NULL);

	/* debug: printf("sbSessLstnSendFram, Frame to send: '%s'\n", pFram->szRawBuf); */

	/* finish the frame */
	pFram->uBytesSend = 0;
	pFram->pChan = pChan;

	/* enqueue it */
	if((pEntry = sbNVTAddEntry(pThis->pSendQue)) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	if((iRet = 	sbNVTESetUsrPtr(pEntry, pFram, (void*) sbSessLstnLinkedListFreeFram)) != SR_RET_OK)
		return iRet;

	return SR_RET_OK;
}



/**
 * Send an reply to the remote peer. Obviously,
 * it is all channel zero stuff.
 *
 * \param pszPayload Message payload.
 */
srRetVal sbSessSendRPY(sbSessObj *pThis, char* pszPayload)
{
	srRetVal iRet;
	sbMesgObj *pMesg;

	sbSessCHECKVALIDOBJECT(pThis);

	if((pMesg = sbMesgConstruct(BEEP_DEFAULT_MIME_HDR, pszPayload)) == NULL)
		return SR_RET_ERR;
	else
	{
		iRet = sbMesgSendMesg(pMesg, pThis->pChan0, "RPY", 0);
		sbMesgDestroy(pMesg);
	}

	return iRet;
}


/**
 * Receives & acts on a channel 0 initialization message
 * (aka "greeting" ;)).
 */
static srRetVal sbSessChan0RecvInitMesg(sbProfObj *pThis, sbSessObj* pSess, sbChanObj *pChan, sbMesgObj *pMesg)
{
	srRetVal iRet;

	sbProfCHECKVALIDOBJECT(pThis);
	sbSessCHECKVALIDOBJECT(pSess);
	sbChanCHECKVALIDOBJECT(pChan);
	sbMesgCHECKVALIDOBJECT(pMesg);

	if(pMesg->idHdr != BEEPHDR_RPY)
		return SR_RET_INVALID_GREETING;

	/* process the greeting */
	if((iRet = sbSessProcessGreeting(pSess, pMesg)) != SR_RET_OK)
		return iRet;

	/* ok, we have done what was to do. So let's update the channel status... */
	pChan->iState = sbChan_STATE_OPEN;

	return SR_RET_OK;
}


/**
 * Retrieve the channel number from an incoming start 
 * or close message.
 */
static srRetVal sbSessGetChannoFromXML(sbSessObj* pSess, unsigned *puChanno, sbProfObj *pProf, sbMesgObj *pMesg, sbNVTEObj *pEntry)
{
	srRetVal iRet;
	sbNVTEObj* pEntryChanno;

	sbProfCHECKVALIDOBJECT(pProf);
	sbSessCHECKVALIDOBJECT(pSess);
	sbMesgCHECKVALIDOBJECT(pMesg);
	sbNVTECHECKVALIDOBJECT(pEntry);

	if((pEntryChanno = sbNVTRHasElement(pEntry->pXMLProps, "number", FALSE)) == NULL)
	{
		sbChanSendErrResponse(pSess->pChan0, 501, "number argument missing in element");
		return SR_RET_START_MISSING_NUMBER;
	}

	iRet = sbNVTEGetValueU(pEntryChanno, puChanno);
	if(iRet == SR_RET_NO_VALUE)
	{
		sbChanSendErrResponse(pSess->pChan0, 501, "number argument invalid in element");
		return SR_RET_START_INVALID_NUMBER;
	}

	return SR_RET_OK;
}


/**
 * Process a channel 0 start message.
 *
 * \param pProf the associated profile
 * \param pSess the associated session
 * \param pMesg the message received
 * \param pEntry pointer to the XML entry of the start element. Can
 *               be used to obtain the "start"-parameters.
 */
static srRetVal sbSessDoStartMesg(sbProfObj *pProf, sbSessObj* pSess, sbMesgObj *pMesg, sbNVTEObj *pEntry)
{
	srRetVal iRet;
	sbNVTEObj* pEntryChanno;
	sbNVTEObj* pProfileEntry;
	sbNVTEObj* pEntryURI;
	sbNVTEObj* pServURIEntry;
	sbProfObj* pProfFound;
	sbChanObj* pChan;
	unsigned uChanno;
	char szRPYBuf[512];

	sbProfCHECKVALIDOBJECT(pProf);
	sbSessCHECKVALIDOBJECT(pSess);
	sbMesgCHECKVALIDOBJECT(pMesg);
	sbNVTECHECKVALIDOBJECT(pEntry);

	/* first, validate the channel number */
	if((pEntryChanno = sbNVTRHasElement(pEntry->pXMLProps, "number", FALSE)) == NULL)
	{
		sbChanSendErrResponse(pSess->pChan0, 501, "number argument missing in start element");
		return SR_RET_START_MISSING_NUMBER;
	}

	iRet = sbNVTEGetValueU(pEntryChanno, &uChanno);
	if(iRet == SR_RET_NO_VALUE)
	{
		sbChanSendErrResponse(pSess->pChan0, 501, "number argument invalid in start element");
		return SR_RET_START_INVALID_NUMBER;
	}

	if((uChanno % 2) != 1)
	{   /* RFC 3080, 2.3.1.2: initiator start numbers must be odd! */
		sbChanSendErrResponse(pSess->pChan0, 501, "number argument in start element must be odd-valued");
		return SR_RET_START_EVEN_NUMBER; 
	}

	/* ok, we got the channel, so let's see if it already exists */
	if(sbSessRetrChanObj(pSess, uChanno) != NULL)
	{
		sbChanSendErrResponse(pSess->pChan0, 550, "requested channel already exists");
		return SR_RET_START_EXISTING_NUMBER;
	}

	/* OK, fine. The new channel number is great and can be 
     * used. So let's go to stage 2, check if the URI exists.
	 * The client can potentially provide multiple URIs. So we
	 * do the following: we first check, if any URIs are provided
	 * at all. If not, an error is raised. If there are URIs, we
	 * walk this list and see if we support any one of this. We
	 * stop as soon as we have found one and accept this. As such,
	 * there is no real selection by profile priority in this version
	 * of the lib - it simply picks the first one asked for. Later
	 * revision may change this. This can become espcially important
	 * when cooked is implemented.
	 */
	if(pEntry->pChild == NULL)
		return SR_RET_NO_PROFILE_RQSTD;

	pProfFound = NULL;
	if(pSess->pProfilesSupported != NULL)
	{	/* safeguard if we have no profiles available */
		pProfileEntry = NULL;
		while(   ((pProfileEntry = sbNVTSearchKeySZ(pEntry->pChild, pProfileEntry, "profile")) != NULL)
			  && (pProfFound == NULL))
			if((pEntryURI = sbNVTRHasElement(pProfileEntry->pXMLProps, "uri", TRUE)) != NULL)
			{
				/* Try to find it. If it finds it, all is done, nothing else necessary. */
				if((pServURIEntry = sbNVTRHasElement(pSess->pProfilesSupported, pEntryURI->pszValue, FALSE)) != NULL)
					pProfFound = (sbProfObj*) pServURIEntry->pUsr;
			}
	}

	if(pProfFound == NULL)
	{
		sbChanSendErrResponse(pSess->pChan0, 550, "no requested profiles are acceptable");
		return SR_RET_WARNING_START_NO_PROFMATCH;
	}
	
	/* OK, finally we have checked everything and it is OK. Now we
	 * can create the new channel and - if all goes well - send a
	 * positive reply to the remote peer.
	 */
	if((pChan = sbChanConstruct(pSess)) == NULL)
	{
		sbChanSendErrResponse(pSess->pChan0, 451, "internal channel object could not be created");
		return SR_RET_OUT_OF_MEMORY;
	}

	if((iRet = sbChanSetChanno(pChan, uChanno)) != SR_RET_OK)
	{
		char szBuf[128];
		SNPRINTF(szBuf, sizeof(szBuf)/sizeof(char), "internal error %d adding channel %u to the session", iRet, uChanno);
		sbChanSendErrResponse(pSess->pChan0, 451, szBuf);
		return SR_RET_OUT_OF_MEMORY;
	}

	pChan->pProf = pProfFound;

	SNPRINTF(szRPYBuf, sizeof(szRPYBuf), "<profile uri='%s' />", pProfFound->pszProfileURI);
	if((iRet = sbSessSendRPY(pSess, szRPYBuf)) != SR_RET_OK)
		return iRet;

	/* Now we must call the new profiles init handler! */
	if(pProfFound->OnChanCreate != NULL)
		if((iRet = pProfFound->OnChanCreate(pProfFound, pSess, pChan)) != SR_RET_OK)
			return iRet;

	return SR_RET_OK;
}


/**
 * Helper to sbSessDoCloseMesg(). This is the callback that
 * will be called once the OK message has been physically
 * sent. It then tears down the channel.
 */
static void sbSessDoChanDestroy(sbFramObj *pFram)
{
	sbChanObj *pChan;

	sbFramCHECKVALIDOBJECT(pFram);
	pChan = (sbChanObj*) pFram->pUsr;
	sbChanCHECKVALIDOBJECT(pChan);

	if(pChan->uChanNum == 0)
	{	/* channel 0 close means session is closed */
		pChan->pSess->iState = sbSESSSTATE_CLOSED;
	}
	sbChanSetChanClosed(pChan);
	sbChanDestroy(pChan);
}


/**
 * Process a channel 0 close message.
 *
 * \param pSess the associated session
 * \param pbAbort [out] set to TRUE if the upper layer should
 *                abort processing this BEEP session. Left
 *                untouched otherwise.
 * \param pProf the associated profile
 * \param pMesg the message received
 * \param pEntry pointer to the XML entry of the close element. Can
 *               be used to obtain the "close"-parameters.
 *
 * \todo See RFC for actual close processing - we do not
 * do this fully correct right at this time! For syslog,
 * it should be sufficient, but there can be interop 
 * issues!
 */
static srRetVal sbSessDoCloseMesg(sbSessObj* pSess, int* pbAbort, sbProfObj *pProf, sbMesgObj *pMesg, sbNVTEObj *pEntry)
{
	srRetVal iRet;
	sbChanObj *pChan;
	unsigned uChanno;

	sbProfCHECKVALIDOBJECT(pProf);
	assert(pbAbort != NULL);
	sbSessCHECKVALIDOBJECT(pSess);
	sbMesgCHECKVALIDOBJECT(pMesg);
	sbNVTECHECKVALIDOBJECT(pEntry);

	if((iRet = sbSessGetChannoFromXML(pSess, &uChanno, pProf, pMesg, pEntry)) != SR_RET_OK)
		return iRet;

	/* find the associated channel & profile */
	if((pChan = sbSessRetrChanObj(pSess, uChanno)) == NULL)
		return SR_RET_CHAN_DOESNT_EXIST;

	/* Please note: we retrieve the channel just to see
	 * if it is valid. We MUST NOT send the <ok/> reply
	 * over this channel - all OK replies MUST travel
	 * over channel zero!
	 */

	/* for now, we assume that we can close the channel
	 * immediately. This is a good assumption for our
	 * logging code, as we can not have any outstanding
	 * msgs. However, it may be worth the do this later a
	 * little more in the spirit of "real BEEP".
	 *
	 * We need to set a callback handler to acutally shut
	 * down the channel. The reason is that due to the select()
	 * nature of our server, data will be send asynchronously and
	 * we can not yet destroy the channel - at least our OK 
	 * message would never be send.
	 */
	if((iRet = sbChanSendOK(pSess->pChan0, sbSessDoChanDestroy, pChan)) != SR_RET_OK)
	{
		*pbAbort = TRUE;
		return iRet;
	}
	
	return SR_RET_OK;
}


/**
 * Receives & acts on a channel 0 payload message.
 * As of now, this can be either an channel start
 * or close request.
 */
static srRetVal sbSessChan0RecvPayloadMesg(sbSessObj* pSess, int *pbAbort, sbProfObj *pProf, sbChanObj *pChan, sbMesgObj *pMesg)
{
	srRetVal iRet;
	sbNVTRObj* pMesgXML;
	sbNVTEObj* pEntry;

	sbProfCHECKVALIDOBJECT(pProf);
	sbSessCHECKVALIDOBJECT(pSess);
	sbChanCHECKVALIDOBJECT(pChan);
	sbMesgCHECKVALIDOBJECT(pMesg);

	/* debug: printf("sbSessChan0RecvPayloadFram(), hdr: '%s', data: '%s'\n", pMesg->szMIMEHdr, pMesg->szActualPayload); */

	if(pMesg->idHdr != BEEPHDR_MSG)
		return SR_RET_INVALID_CHAN0_MESG;

	/* look into the message */
	pMesgXML = sbNVTRConstruct();
	if((iRet = sbNVTRParseXML(pMesgXML, pMesg->szActualPayload)) != SR_RET_OK)
	{
		sbNVTRDestroy(pMesgXML);
		return iRet;
	}

	if((pEntry = pMesgXML->pFirst) == NULL)
	{ 
		sbNVTRDestroy(pMesgXML);
		return SR_RET_INVALID_CHAN0_MESG;
	}

	if(!strcmp(pEntry->pszKey, "start"))
	{
		if((iRet = sbSessDoStartMesg(pProf, pSess, pMesg, pEntry)) != SR_RET_OK)
		{ 
			sbNVTRDestroy(pMesgXML);
			return iRet;
		}
	}
	else if(!strcmp(pEntry->pszKey, "close"))
	{
		if((iRet = sbSessDoCloseMesg(pSess, pbAbort, pProf, pMesg, pEntry)) != SR_RET_OK)
		{ 
			sbNVTRDestroy(pMesgXML);
			return iRet;
		}

	}
	else
	{ 
		sbNVTRDestroy(pMesgXML);
		return SR_RET_INVALID_CHAN0_MESG;
	}

	sbNVTRDestroy(pMesgXML);
	return SR_RET_OK;
}


/**
 * Channel 0 "profile" receive frame event handler.
 * This handler is used to drive channel 0 frames. It is
 * responsible for all channel startup and closedown.
 */
static srRetVal sbSessChan0OnRecvMesg(sbProfObj *pProf, int *pbAbort, sbSessObj* pSess, sbChanObj *pChan, sbMesgObj *pMesg)
{
	srRetVal iRet;
	sbProfCHECKVALIDOBJECT(pProf);
	sbSessCHECKVALIDOBJECT(pSess);
	sbChanCHECKVALIDOBJECT(pChan);
	sbMesgCHECKVALIDOBJECT(pMesg);
	assert(pbAbort != NULL);

	switch(pChan->iState)
	{
	case sbChan_STATE_INITIALIZED:
		if((iRet = sbSessChan0RecvInitMesg(pProf, pSess, pChan, pMesg)) != SR_RET_OK)
			return iRet;
		break;
	case sbChan_STATE_OPEN:
		if((iRet = sbSessChan0RecvPayloadMesg(pSess, pbAbort, pProf, pChan, pMesg)) != SR_RET_OK)
			return iRet;
		break;
	default:
		return SR_RET_INVALID_CHAN_STATE;
	}

	return SR_RET_OK;
}


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */


srRetVal sbSessRemoteOpen(sbSessObj **pThis, sbSockObj *pSock, sbNVTRObj *pProfSupported)
{
	sbProfObj *pProf;
	srRetVal iRet;
	assert(pThis != NULL);

	if((*pThis = calloc(1, sizeof(sbSessObj))) == NULL)
	{
		return SR_RET_OUT_OF_MEMORY;
	}

	(*pThis)->iState = sbSESSSTATE_UNKNOWN;
	(*pThis)->OID = OIDsbSess;
	(*pThis)->pSendQue = NULL;
	(*pThis)->pRemoteProfiles = NULL;
	(*pThis)->pRXQue = NULL;
	(*pThis)->pSendQue = NULL;
	(*pThis)->pSock = pSock;
	(*pThis)->SendFramMethod = sbSessLstnSendFram;
	(*pThis)->bNeedData = FALSE;
	(*pThis)->pProfilesSupported = pProfSupported;

	/* Now build the list of channels.
	 */
	if(((*pThis)->pChannels = sbNVTRConstruct()) == NULL)
	{
		sbSessDestroy(*pThis);
		*pThis = NULL;
		return SR_RET_OUT_OF_MEMORY;
	}

	/* build the send queue */
	if(((*pThis)->pSendQue = sbNVTRConstruct()) == NULL)
	{
		sbSessDestroy(*pThis);
		*pThis = NULL;
		return SR_RET_OUT_OF_MEMORY;
	}

	/* Ok, we got a connection. Now we need to create a channel object for channel
	 * 0, so that we can begin exchanging greetings. The greeting exchange itself
	 * is done in a separate method.
	 */
	(*pThis)->pChan0 = sbChanConstruct(*pThis);

	/* now create profile for channel 0 protocol & assign it to the channel */
	if((iRet = sbProfConstruct(&pProf, NULL)) != SR_RET_OK)
	{
		sbSessDestroy(*pThis); /* best we can do */
		*pThis = NULL;
		return iRet;
	}
	pProf->bDestroyOnChanClose = TRUE;

	if((iRet = sbProfSetEventHandler(pProf, sbPROFEVENT_ONMESGRECV, (void*) sbSessChan0OnRecvMesg)) != SR_RET_OK)
	{
		sbSessDestroy(*pThis); /* best we can do */
		*pThis = NULL;
		return iRet;
	}

	if((iRet = sbChanAssignProfile((*pThis)->pChan0, pProf)) != SR_RET_OK)
	{
		sbSessDestroy(*pThis); /* best we can do */
		*pThis = NULL;
		return iRet;
	}

	sbChanSetChanno((*pThis)->pChan0, 0);	/* this is always channel 0 by RFC */

	return SR_RET_OK;
}


void sbSessAbort(sbSessObj *pThis)
{
	sbNVTEObj *pEntry;
	sbSessCHECKVALIDOBJECT(pThis);

	/* First of all, we must indicate on all channels
	 * that they are being aborted. This will instruct
	 * the sbChanTeardown() callback to just delete the
	 * channel object but not try to close them correctly.
	 */
	pEntry = sbNVTSearchKeySZ(pThis->pChannels, NULL, NULL);
	while(pEntry != NULL)
	{
		if(pEntry->pUsr != NULL)
		{
			sbChanCHECKVALIDOBJECT((sbChanObj*) pEntry->pUsr);
			((sbChanObj*) pEntry->pUsr)->iState = sbChan_STATE_ERR_FREE_NEEDED;
		}
		pEntry = sbNVTSearchKeySZ(pThis->pChannels, pEntry, NULL);
	}

	/* we now need to tear down the TCP stream */
	sbSockExit(pThis->pSock);
	sbSessDestroy(pThis);
}
