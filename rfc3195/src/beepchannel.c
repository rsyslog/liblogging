/*! \file beepchannel.c
 *  \brief The Implementation of the BEEP Channel object
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 *
 * \date    2003-08-07
 *          - added support for the session's channel linked lies.
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
#include "beepchannel.h"
#include "beepframe.h"
#include "namevaluetree.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */


/**
 * This method is used to tear down a channel. It results in 
 * ultimate destruction of the channel. This method has been 
 * created to fulfill the requirement of sbNVTESetUsrPtr, which
 * must have a destructor. And, well, it should work well but so far
 * has not really been tested... ;)
 */
static void sbChanTeardown(sbChanObj *pThis)
{
	sbChanCHECKVALIDOBJECT(pThis);

	if((pThis->iState == sbChan_STATE_OPEN))
	 	sbSessCloseChan(pThis->pSess, pThis);
	else if(pThis->iState == sbChan_STATE_ERR_FREE_NEEDED)
		sbChanAbort(pThis);
	/* Any other states currently mean the caller has already
	 * closed the channel and also free()ed the object. So this is
	 * really an artefact. This is to cover some race conditions.
	 * We need to go back at this code over time and do a
	 * thourough audit. Obviously, we are dealing with a
	 * free()ed memory block here in some instances, which
	 * is not a good thing to do.
	 */

	/** \todo take care of the pending_close state here (one we do this) */
}


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */


srRetVal sbChanSetChanno(sbChanObj *pThis, int iChanno)
{
	sbNVTEObj *pEntry;
	srRetVal iRet = SR_RET_OK; 

	sbChanCHECKVALIDOBJECT(pThis);
	pThis->uChanNum = iChanno;

	/* Now add this channel to the session's linked list.
	 * Be sure to read the method description in beepchannel.h
	 * before touching this code!!!!
	 */
	if((pEntry = sbNVTAddEntry(pThis->pSess->pChannels)) == NULL)
		return SR_RET_ERR;

	sbNVTESetKeyU(pEntry, iChanno);
	sbNVTESetUsrPtr(pEntry, pThis, (void*) sbChanTeardown);

	return iRet;
}


sbChanObj* sbChanConstruct(sbSessObj* pSess)
{
	sbChanObj* pThis;
	sbSessCHECKVALIDOBJECT(pSess);

	if((pThis = calloc(1, sizeof(sbChanObj))) == NULL)
		return NULL;

	pThis->pSess = pSess;
	pThis->pSock = pSess->pSock;
	pThis->uChanNum = SBCHAN_INVALID_CHANNEL;
	pThis->uRXWin = BEEP_DEFAULT_WINDOWSIZE;
	pThis->uTXWin = BEEP_DEFAULT_WINDOWSIZE;
	pThis->uTXWinLeft = BEEP_DEFAULT_WINDOWSIZE;
	pThis->uRXWinLeft = BEEP_DEFAULT_WINDOWSIZE;
	pThis->uSeqno = 0;
	pThis->uMsgno = 0;
	pThis->OID = OIDsbChan;
	pThis->iState = sbChan_STATE_INITIALIZED;
	pThis->pProfInstance = NULL;
	pThis->pProf = NULL;

	return(pThis);
}


srRetVal sbChanActualSendFram(sbChanObj *pThis, sbFramObj* pFram)
{
	unsigned iPayloadLen;
	unsigned iFrameLen;

	sbChanCHECKVALIDOBJECT(pThis);
	sbFramCHECKVALIDOBJECT(pFram);

	iFrameLen = sbFramGetFrameLen(pFram); /* physical size including header */
	iPayloadLen = pFram->uSize;	/* logical size, THIS must be used for the 3081 window! */
	if(iPayloadLen > pThis->uTXWinLeft)
		return SR_RET_REMAIN_WIN_TOO_SMALL;

	if(sbSockSend(pThis->pSock, sbFramGetFrame(pFram), iFrameLen) != iFrameLen)
		return SR_RET_SOCKET_ERR;

	pThis->uTXWinLeft -= iPayloadLen;
	pFram->iState = sbFRAMSTATE_SENT;
	
	return SR_RET_OK;
}


#	if FEATURE_LISTENER == 1
void sbChanAbort(sbChanObj* pThis)
{
	sbChanCHECKVALIDOBJECT(pThis);

	if(pThis->pProf != NULL)
		if(pThis->pProf->bDestroyOnChanClose == TRUE)
		{
			sbProfDestroy(pThis->pProf);
		}
	SRFREEOBJ(pThis);
}
#	endif

void sbChanDestroy(sbChanObj* pThis)
{
#	if FEATURE_LISTENER == 1
	sbProfObj *pProf;
#	endif
	sbChanCHECKVALIDOBJECT(pThis);

	/* This is a safeguard to make sure no
	 * leak is left. Normally, the profile cleans
	 * up this pointer. But it can't do so if e.g.
	 * the channel is aborted. So we do it for it,
	 * but obiously, we can't do it smartly - so we
	 * just free() the memory.
	 */
	if(pThis->pProfInstance != NULL)
		free(pThis->pProfInstance);

#	if FEATURE_LISTENER == 1
		pProf = pThis->pProf;
#	endif
	/* we now need to remove the channel object from the session's
	 * channel list. 
	 */
	sbNVTRRemoveKeyU(pThis->pSess->pChannels, pThis->uChanNum);

#	if FEATURE_LISTENER == 1
	if(pProf != NULL)
		if(pProf->bDestroyOnChanClose == TRUE)
			sbProfDestroy(pProf);
#	endif

	SRFREEOBJ(pThis);
}


srRetVal sbChanUpdateChannelState(sbChanObj* pThis, int iNewState)
{
	sbChanCHECKVALIDOBJECT(pThis);
	
	pThis->iState = iNewState;

	return SR_RET_OK;
}

/* ######################### LISTENER ONLY ############################### */
#if FEATURE_LISTENER == 1

srRetVal sbChanAssignProfile(sbChanObj *pThis, sbProfObj *pProf)
{	
	sbChanCHECKVALIDOBJECT(pThis);
	sbProfCHECKVALIDOBJECT(pProf);

	if(pThis->pProf != NULL)
		return SR_RET_PROFILE_ALREADY_SET;

	pThis->pProf = pProf;

	return SR_RET_OK;
}


srRetVal sbChanSetAwaitingClose(sbChanObj* pThis)
{
	sbChanCHECKVALIDOBJECT(pThis);

	pThis->iState = sbChan_STATE_AWAITING_CLOSE;

	return SR_RET_OK;
}

srRetVal sbChanSetChanClosed(sbChanObj* pThis)
{
	sbChanCHECKVALIDOBJECT(pThis);

	pThis->iState = sbChan_STATE_CLOSED;

	return SR_RET_OK;
}


srRetVal sbChanSendOK(sbChanObj*pThis, void (*OnFrameDestroy)(sbFramObj*), void*pUsr)
{
	srRetVal iRet;
	sbMesgObj *pMesg;

	sbChanCHECKVALIDOBJECT(pThis);

	pMesg = sbMesgConstruct(BEEP_DEFAULT_MIME_HDR, "<ok />\r\n");
	iRet = sbMesgSendMesgWithCallback(pMesg, pThis, "RPY", 0, OnFrameDestroy, pUsr);
	sbMesgDestroy(pMesg);

	return iRet;
}


srRetVal sbChanSendSEQ(sbChanObj *pThis, unsigned uAckno, unsigned uWindow)
{
	srRetVal iRet;
	sbFramObj *pFram;

	sbChanCHECKVALIDOBJECT(pThis);

	if((iRet = sbFramCreateSEQFram(&pFram, pThis, uAckno, 0)) != SR_RET_OK)
		return iRet;
	
	/* frame constructed, now send it */
	iRet = sbFramSendFram(pFram, pThis);
	
	if(pFram->iState == sbFRAMSTATE_SENT)
		sbFramDestroy(pFram);

	return(iRet);
}


srRetVal sbChanSendErrResponse(sbChanObj *pThis, unsigned uErrCode, char* pszErrMsg)
{
	srRetVal iRet;
	sbMesgObj *pMesg;
	char szPayload[1025];
	char *pszErrEscaped;

	sbChanCHECKVALIDOBJECT(pThis);
	assert(pszErrMsg != NULL);
	assert(uErrCode != 0); /* this can be a typical error! */
	assert(strlen(pszErrMsg) < 900);

#	if SECURITY_PEER_ERRREPORT_LEVEL == 0
		strcpy(szPayload, "<error code='550'>error</error>\r\n");
#	endif
#	if SECURITY_PEER_ERRREPORT_LEVEL == 1
		if(uErrCode == 451)
			strcpy(szPayload, "<error code='550'>error occured</error>\r\n");
		else
		{
			if((pszErrEscaped = sbNVTXMLEscapePCDATA(pszErrMsg)) == NULL)
				strcpy(szPayload, "<error code='550'>error occured</error>\r\n");
			else
			{
				SNPRINTF(szPayload, sizeof(szPayload), "<error code='%u'>%s</error>\r\n", uErrCode, pszErrEscaped);
				free(pszErrEscaped);
			}
		}
#	endif
#	if SECURITY_PEER_ERRREPORT_LEVEL == 2
			if((pszErrEscaped = sbNVTXMLEscapePCDATA(pszErrMsg)) == NULL)
				strcpy(szPayload, "<error code='%u'>Memory shortage - actual error message could not be generated!\r\nThe error code, however, is correct.</error>\r\n", uErrCode);
			else
			{
				SNPRINTF(szPayload, sizeof(szPayload), "<error code='%u'>%s</error>\r\n", uErrCode, pszErrEscaped);
				free(pszErrEscaped);
			}
#	endif

	if((pMesg = sbMesgConstruct(BEEP_DEFAULT_MIME_HDR, szPayload)) == NULL)
		/* there is not much we can do in this case... (and it is unlikely, too) */
		return SR_RET_OUT_OF_MEMORY;
	
	iRet = sbMesgSendMesg(pMesg, pThis, "ERR", 0);
	sbMesgDestroy(pMesg);
	
	return iRet;
}

#endif
/* ####################### END LISTENER ONLY ############################# */
