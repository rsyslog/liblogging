/*! \file beepsession.c
 *  \brief Implementation of the BEEP Session Object.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 * \date    2003-08-11
 *			Integrated XML parser in closeSession and other
 *          areas (begun implementing it).
 * \date	2003-08-12
 *			Added a list of remote peers profile to the session
 *          object. Now the sbSessOpenChan() can do intelligent
 *          checking if the remote peer supports the profile 
 *          (and, if not, provide an error message and not
 *          try to connect).
 * \date	2003-09-04
 *          Added support for multiple initiator (client) profiles.
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
#include "stringbuf.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */



/**
 * Retrieve the channel object associated with the channel number.
 * This method allows to retrieve a channel number based on the 
 * actual channel.
 * 
 * \param uChanNo channel number
 * \retval pointer to channel object or NULL, if uChanNo could
 *         not be found.
 */
sbChanObj* sbSessRetrChanObj(sbSessObj *pThis, SBchannel uChanNo)
{
	sbNVTEObj *pEntry;

	if((pEntry = sbNVTSearchKeyU(pThis->pChannels, NULL, uChanNo)) == NULL)
		return NULL;

	return((sbChanObj*) pEntry->pUsr);
}


srRetVal sbSessProcessGreeting(sbSessObj *pThis, sbMesgObj *pMesg)
{
	sbNVTRObj* pMesgXML;
	sbNVTEObj* pEntryGreeting;
	sbNVTRObj* pProfileRoot;
	sbNVTEObj* pProfileEntry;
	sbNVTRObj* pProfileList = NULL;
	sbNVTEObj* pProfileListEntry;
	sbNVTEObj* pEntryURI;
	srRetVal iRet;

	sbSessCHECKVALIDOBJECT(pThis);
	sbMesgCHECKVALIDOBJECT(pMesg);

	/* Now parse the greeting and see which profiles are supported.
	 * The profiles supported by the remote peer are stored in the
	 * profiles linked list which is referred to when a channel
	 * creation is attempted.
	 */
	pMesgXML = sbNVTRConstruct();

	if((iRet = sbNVTRParseXML(pMesgXML, pMesg->szActualPayload)) == SR_RET_OK) {
		if((pEntryGreeting = sbNVTRHasElement(pMesgXML, "greeting", TRUE)) == NULL)
			iRet = SR_RET_PEER_NO_GREETING;
		else
		{	/* ok, we now have a correct greeting message. Now let's pick up all
			 * the profiles from it. It is no error if we do not find any profile
			 * object, as this is valid as of RFC 3080 (of course, it makes no
			 * sense when talking to a BEEP listener, but that will be handled
			 * at channel creation). If we find profile elements but they do not
			 * include the uri parameter, we raise an error, as this is not valid
			 * as of the DTD. Other profile parameters are not used and ignored.
			 */
			pProfileList = NULL;
			if((pProfileRoot = pEntryGreeting->pChild) != NULL)
			{
				pProfileEntry = NULL;
				while((pProfileEntry = sbNVTSearchKeySZ(pProfileRoot, pProfileEntry, "profile")) != NULL)
				{
					if((pEntryURI = sbNVTRHasElement(pProfileEntry->pXMLProps, "uri", TRUE)) == NULL)
					{
						sbNVTRDestroy(pMesgXML);
						iRet = SR_RET_PEER_NO_URI;
						break;
					}
					else
					{	/* ok, we finally have a valid profile. Let's add it to our list. */
						if(pProfileList == NULL)
							if((pProfileList = sbNVTRConstruct()) == NULL)
							{
								sbNVTRDestroy(pMesgXML);
								iRet = SR_RET_OUT_OF_MEMORY;
								break;
							}
						if((pProfileListEntry = sbNVTAddEntry(pProfileList)) == NULL)
						{
							sbNVTRDestroy(pMesgXML);
							iRet = SR_RET_OUT_OF_MEMORY;
							break;
						}
						sbNVTESetKeySZ(pProfileListEntry, pEntryURI->pszValue, TRUE);
					}
				}
			}
		}
	}

	pThis->pRemoteProfiles = pProfileList;
	sbNVTRDestroy(pMesgXML);

	return iRet;
}


sbSessObj* sbSessOpenSession(char* pszRemotePeer, int iPort, sbNVTRObj *pProfSupported)
{
	sbSockObj *pSock;
	sbSessObj* pThis;
	sbChanObj* pChan0;
	sbMesgObj* pMesgGreeting;
	sbMesgObj* pGreeting;
	srRetVal iRet;

	if((pThis = calloc(1, sizeof(sbSessObj))) == NULL)
	{
		return NULL;
	}

	sbSessResetLastError(pThis);

	/* Now build the list of channels.
	 */
	if((pThis->pChannels = sbNVTRConstruct()) == NULL)
	{
		SRFREEOBJ(pThis);
		return NULL;
	}

	pSock = sbSockInit();
	if(pSock == NULL)
	{
		SRFREEOBJ(pThis);
		return NULL;
	}

	if(sbSockConnectoToHost(pSock, pszRemotePeer, iPort) != SR_RET_OK)
	{
		/* debug:		printf("Error, can not connect to host!\n"); */
		sbSockExit(pSock);
		SRFREEOBJ(pThis);
		return NULL;
	}

	/* finish up */
	pThis->pProfilesSupported = pProfSupported;
	pThis->pSock = pSock;
	pThis->OID = OIDsbSess;
	pThis->SendFramMethod = sbSessSendFram;
	if((pThis->pRXQue = sbNVTRConstruct()) == NULL)
		return NULL;

	/* Ok, we got a connection. Now we need to create a channel object for channel
	 * 0, so that we can begin exchanging greetings.
	 */
	pMesgGreeting = sbMesgConstruct("Content-type: application/beep+xml\r\n",
									"<greeting />\r\n");
	pChan0 = sbChanConstruct(pThis);
	sbChanSetChanno(pChan0, 0);	/* this is always channel 0 by RFC */
	pThis->pChan0 = pChan0;

	sbMesgSendMesg(pMesgGreeting, pChan0, "RPY", 0);
	sbMesgDestroy(pMesgGreeting);

	if((pGreeting = sbMesgRecvMesg(pChan0)) == NULL)
	{	/* we failed, so free as much as possible */
		sbSessCloseSession(pThis);
		return NULL;
	}

	if((iRet = sbSessProcessGreeting(pThis, pGreeting)) != SR_RET_OK)
	{	/* we failed, so free as much as possible */
		sbSessSetLastError(pThis, iRet);
		sbSessCloseSession(pThis);
		sbMesgDestroy(pGreeting);
		return NULL;
	}

	sbMesgDestroy(pGreeting);
	return(pThis);
}


/**
 * Process the supplied SEQ frame. Most notably, this means the
 * window is updated in the respective channel object.
 *
 * According to the latest discussion on the BEEP WG mailing 
 * list and other sources, we silently ignore SEQ frames for
 * non-existing channels. This most probably is not a protocol
 * error but rather a SEQ frame arriving after the channel
 * has been closed. This can happen due to TCP delays. 
 * Other implementors have suggested (and successfully 
 * implemented) ignoring these frames. This information is
 * current as of August, 2003.
 *
 * \param pFram pointer to frame to process.
 */
srRetVal sbSessDoSEQ(sbSessObj* pThis, sbFramObj* pFram)
{
	sbChanObj *pChan;
	int uConsumed;

	sbSessCHECKVALIDOBJECT(pThis);
	assert(pFram->idHdr = BEEPHDR_SEQ);

	if((pChan = sbSessRetrChanObj(pThis, pFram->uChannel)) == NULL)
		return SR_RET_OK; /* see above for an explanation */

	/* OK, we have all we need and now can do the math */
	uConsumed = pChan->uSeqno - pFram->uAckno + 1;
	pChan->uTXWin = pFram->uWindow;
	pChan->uTXWinLeft = pChan->uTXWin - uConsumed;

	return SR_RET_OK;
}


void sbSessDestroy(sbSessObj *pThis)
{
	sbSessCHECKVALIDOBJECT(pThis);
	if(pThis->pRXQue != NULL)
		sbNVTRDestroy(pThis->pRXQue);
	if(pThis->pRemoteProfiles != NULL)
		sbNVTRDestroy(pThis->pRemoteProfiles);
	if(pThis->pChannels != NULL)
		sbNVTRDestroy(pThis->pChannels);
#	if FEATURE_LISTENER == 1
	/* NOTE WELL: we do NOT destroy the pProfilesSupported
	 * NameValueTree because this is a read-only copy provided
	 * by either the sbLstnObj or the srAPI object. These are
	 * responsible for destroying it.
	 */
	if(pThis->pSendQue != NULL)
		sbNVTRDestroy(pThis->pSendQue);
#	endif
	SRFREEOBJ(pThis);
}


srRetVal sbSessDoReceive(sbSessObj *pThis, int bMustRcvPayloadFrame)
{
	sbFramObj* pFram;
	srRetVal iRet;
	sbNVTEObj *pEntry;

	do
	{
		if((pFram = sbFramActualRecvFram(pThis)) == NULL)
			return SR_RET_ERR;

		/* let's see if it is a SEQ frame */
		if(sbFramGetHdrID(pFram) == BEEPHDR_SEQ)
		{
			if((iRet = sbSessDoSEQ(pThis, pFram)) != SR_RET_OK)
				return iRet;
			/* frame is now eaten up, so destroy */
			sbFramDestroy(pFram);
			pFram = NULL;
		}
		else
		{	/* it's an actual data frame - so enqueue it */
			if((pEntry = sbNVTAddEntry(pThis->pRXQue)) == NULL)
				return SR_RET_OUT_OF_MEMORY;

			if((iRet = sbNVTESetUsrPtr(pEntry, pFram, (void*) sbFramDestroy)) != SR_RET_OK)
				return iRet;
		}
	}
	while((bMustRcvPayloadFrame == TRUE) && (pThis->pRXQue->pFirst == NULL));	/* WARNING: do...while */

	return SR_RET_OK;
}


sbFramObj* sbSessRecvFram(sbSessObj* pThis, sbChanObj *pChan)
{
	srRetVal iRet;
	sbFramObj *pRetFram;
	sbNVTEObj *pEntry;

	do
	{
		if(sbSockHasReceiveData(pThis->pSock))
			if((iRet = sbSessDoReceive(pThis, TRUE)) != SR_RET_OK)
				return NULL;

		if((pEntry = sbNVTUnlinkElement(pThis->pRXQue)) == NULL)
		{	/* wait until new data arrives */
			/** \todo think about a timeout here - we could block indefinitely... */
			sbSockWaitReceiveData(pThis->pSock);
		}
	}
	while(pEntry == NULL);	/* WARNING: do...while! */

	pRetFram = pEntry->pUsr;
	sbNVTEUnsetUsrPtr(pEntry);
	sbNVTEDestroy(pEntry);

	/* We now need to check if it is time to send a SEQ ourselfs ... */
	pChan->uRXWinLeft -= pRetFram->uSize;
	if(pChan->uRXWinLeft < BEEP_DEFAULT_WINDOWSIZE / 2)
	{
		pChan->uRXWinLeft = BEEP_DEFAULT_WINDOWSIZE;
		if((iRet = sbChanSendSEQ(pChan, pRetFram->uSeqno+pRetFram->uSize, 0)) != SR_RET_OK)
		{
			sbFramDestroy(pRetFram);	/* out of options... */
			return NULL;
		}
	}

	return(pRetFram);
}


srRetVal sbSessSendFram(sbSessObj *pThis, sbFramObj *pFram, sbChanObj *pChan)
{
	srRetVal iRet;
	int bRetry;

	sbSessCHECKVALIDOBJECT(pThis);
	sbFramCHECKVALIDOBJECT(pFram);
	sbChanCHECKVALIDOBJECT(pChan);

	if(sbSockHasReceiveData(pThis->pSock))
		sbSessDoReceive(pThis, FALSE);

	do
	{
		iRet = sbChanActualSendFram(pChan, pFram);
		
		if(iRet == SR_RET_OK)
			bRetry = FALSE;
		else if(iRet == SR_RET_REMAIN_WIN_TOO_SMALL)
		{ /* we now need to wait for a larger frame */
			if((iRet = sbSessDoReceive(pThis, FALSE)) != SR_RET_OK)	/* do a blocking read */
				return iRet;
			bRetry = TRUE;
		}
		else
			return iRet;
	}
	while(bRetry == TRUE);	/* WARNING: do...while! */

	return SR_RET_OK;
}


sbChanObj* sbSessOpenChan(sbSessObj* pThis)
{
	sbChanObj* pChan;
	sbNVTRObj *pReplyXML;
	sbNVTEObj *pEntryURI;
	sbNVTEObj *pEntryProfile;
	sbMesgObj *pMesgStart;
	sbMesgObj* pReply; /* the reply to the start command */
	srRetVal iRet;
	sbProfObj * pProf;
	char szGreeting[512];

	sbSessCHECKVALIDOBJECT(pThis);

	sbSessResetLastError(pThis);

	/* before we do anything, let's first check if the remote peer
	 * supports at least one of the profiles we do support. If there
	 * is no match, there is no point in trying it...
	 */
	if((pProf = sbProfFindProfileURIMatch(pThis->pProfilesSupported, pThis->pRemoteProfiles)) == NULL)
	{
		sbSessSetLastError(pThis, SR_RET_PEER_DOESNT_SUPPORT_PROFILE);
		return NULL;
	}

	pChan = sbChanConstruct(pThis);

	/* OK, we have our representation of the channel. Now we need 
	 * to see if the remote peer accepts our creation request.
	 */
	SNPRINTF(szGreeting, sizeof(szGreeting),
						"<start number='1'>\r\n"
						"  <profile uri='%s' />\r\n"
						"</start>\r\n", sbProfGetURI(pProf));
	
	pMesgStart = sbMesgConstruct("Content-type: application/beep+xml\r\n",
								  szGreeting);
	sbChanSetChanno(pChan, 1);	/* quick hack */

	sbMesgSendMesg(pMesgStart, pThis->pChan0, "MSG", 0);
	sbMesgDestroy(pMesgStart);

	if((pReply = sbMesgRecvMesg(pChan)) == NULL)
	{
		/** \todo think if we need to shut down anything
		 *  else inside the BEEP session.
		 */
		sbChanDestroy(pChan);
		return NULL;
	}
	/* debug: printf("HDR: %s, Payload: %s\n", pReply->szMIMEHdr, pReply->szActualPayload); */

	if(pReply->idHdr != BEEPHDR_RPY)
	{
		/** \todo think if we need to shut down anything
		 *  else inside the BEEP session.
		 */
		sbChanDestroy(pChan);
		sbMesgDestroy(pReply);
		return NULL;
	}

	/* Now check if we received the correct reply and extract the
	 * agreed-upon profile.
	 */
	pReplyXML = sbNVTRConstruct();

	if((iRet = sbNVTRParseXML(pReplyXML, pReply->szActualPayload)) == SR_RET_OK) {
		if((pEntryProfile = sbNVTRHasElement(pReplyXML, "profile", TRUE)) == NULL) {
			iRet = SR_RET_PEER_NO_PROFILE;
		} else {
			if((pEntryURI = sbNVTRHasElement(pEntryProfile->pXMLProps, "uri", TRUE)) == NULL)
				iRet = SR_RET_PEER_NO_URI;
		}
	}

	if(iRet == SR_RET_OK)
	{
		if((pChan->pProf = sbProfFindProfile(pThis->pProfilesSupported, pEntryURI->pszValue)) == NULL)
			iRet = SR_RET_PEER_INVALID_PROFILE;
	}

	/* housekeeping */
	sbMesgDestroy(pReply);
    sbNVTRDestroy(pReplyXML); /* done with reply, no longer needed */

	if(iRet != SR_RET_OK)
	{
        sbChanDestroy(pChan);
		pChan = NULL;
		sbSessSetLastError(pThis, iRet);
	}

	return(pChan);
}


srRetVal sbSessCloseChan(sbSessObj *pThis, sbChanObj *pChan)
{
	sbMesgObj* pMesgCloseReq;
	sbMesgObj* pMesgCloseReply;
	srRetVal iRet = SR_RET_OK;
	sbNVTRObj *pReplyXML;
	char szBuf[1025];

	sbSessCHECKVALIDOBJECT(pThis);
	sbChanCHECKVALIDOBJECT(pChan);

	SNPRINTF(szBuf, sizeof(szBuf), "<close number='%d' code='200' />", pChan->uChanNum);
	if((pMesgCloseReq = sbMesgConstruct("Content-type: application/beep+xml\r\n",
		szBuf)) == NULL)
		return SR_RET_ERR;

	iRet = sbMesgSendMesg(pMesgCloseReq, pThis->pChan0, "MSG", 0);
	sbMesgDestroy(pMesgCloseReq);

	if(iRet != SR_RET_OK)
		return iRet;

	if((pMesgCloseReply = sbMesgRecvMesg(pThis->pChan0)) == NULL)
	{
		/* this is the best we can do, otherwise we may create a leak... */
		sbChanUpdateChannelState(pChan, sbChan_STATE_CLOSED);
		sbChanDestroy(pChan);
		return iRet;
	}

	if(pMesgCloseReply->idHdr != BEEPHDR_RPY)
	{
		iRet = SR_RET_ERR;
	}

	pReplyXML = sbNVTRConstruct();
	if((iRet = sbNVTRParseXML(pReplyXML, pMesgCloseReply->szActualPayload)) == SR_RET_OK)
		if(sbNVTRHasElement(pReplyXML, "ok", TRUE) == NULL)
			iRet = SR_RET_PEER_NONOK_RESPONSE;

	sbNVTRDestroy(pReplyXML);

	sbMesgDestroy(pMesgCloseReply);
	sbChanUpdateChannelState(pChan, sbChan_STATE_CLOSED);
	sbChanDestroy(pChan);

	return(iRet);
}


srRetVal sbSessCloseSession(sbSessObj *pThis)
{
	srRetVal iRet;

	sbSessCHECKVALIDOBJECT(pThis);

	iRet = sbSessCloseChan(pThis, pThis->pChan0);
	pThis->pChan0 = NULL;	/* ok, will be destroyed soon
							   but I prefer to have it done.. */

	/* we now need to tear down the TCP stream */
	sbSockExit(pThis->pSock);
	sbSessDestroy(pThis);

	return iRet;
}



srRetVal sbSessSendGreeting(sbSessObj* pSess, sbNVTRObj *pProfsSupported)
{
	srRetVal iRet;
	sbMesgObj *pMesgStart;
	sbStrBObj *pStr;
	sbNVTEObj *pEntry;
	char* pBuf;
	char szURIBuf[1025];

	sbSessCHECKVALIDOBJECT(pSess);
	sbNVTRCHECKVALIDOBJECT(pProfsSupported);

	if((pStr = sbStrBConstruct()) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	/* begin greeting */
	if((iRet = sbStrBAppendStr(pStr, "<greeting>\r\n")) != SR_RET_OK)
		return iRet;

	/* walk the profiles and add them to the greeting */
	pEntry = NULL;
	while((pEntry = sbNVTSearchKeySZ(pSess->pProfilesSupported, pEntry, NULL)) != NULL)
	{
		SNPRINTF(szURIBuf, sizeof(szURIBuf), "  <profile uri='%s' />\r\n", sbProfGetURI((sbProfObj*) pEntry->pUsr));
		if((iRet = sbStrBAppendStr(pStr, szURIBuf)) != SR_RET_OK)
			return iRet;
	}

	/* finish greeting */
	if((iRet = sbStrBAppendStr(pStr, "</greeting>\r\n")) != SR_RET_OK)
		return iRet;
	pBuf = sbStrBFinish(pStr);

	pMesgStart = sbMesgConstruct(BEEP_DEFAULT_MIME_HDR, pBuf);
	sbMesgSendMesg(pMesgStart, pSess->pChan0, "RPY", 0);
	sbMesgDestroy(pMesgStart);
	free(pBuf);

	return SR_RET_OK;
}
