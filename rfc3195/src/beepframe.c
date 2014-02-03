/*! \file beepframe.c
 *  \brief Implementation of the BEEP Frame object.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
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
#include "beepmessage.h"
#include "beepframe.h"
#include "beepchannel.h"
#include "beepsession.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */


/**
 * Receive an unsigned integer from the channel.
 * Eats up characters as it processes them.
 *
 * \retval integer receive. If there are no digits
 *         immediately in the input stream, 0 is 
 *         returned and no character is taken from
 *         the input stream.
 */
unsigned sbFramRecvUnsigned(sbSockObj* pSock)
{
	unsigned u = 0;
	char c;

	for(c = sbSockPeekRcvChar(pSock); isdigit(c) ; c = sbSockPeekRcvChar(pSock))
	{
		c = sbSockGetRcvChar(pSock);
		u = u * 10 + c - '0';
	}

	return(u);
}

/**
 * Helper to sbFramActualRecvFram; receives an
 * SEQ frame off the wire. See RFC 3081, 3.1.3 for
 * the semantics of a SEQ frame.
 *
 * \param pThis [in/out] frame object to be filled. This is partly
 *                        created by the caller. new properties read
 *                        by this function are filled in. We do it
 *                        that way so that we can e.g. do error checking
 *                        for calloc() failures only in the caller. Saves
 *                        us some code ;)
 * \param pChan [in/out] the channel to receive the frame on.
 *                       The channel object is updated, e.g. with a
 *                       new seqno.
 */
srRetVal sbFramActualRecvFramSEQ(sbFramObj* pThis, sbChanObj *pChan)
{
	pThis->uAckno = sbFramRecvUnsigned(pChan->pSock);

	if(sbSockGetRcvChar(pChan->pSock) != ' ')
		return SR_RET_ERR;
	pThis->uWindow = sbFramRecvUnsigned(pChan->pSock);

	if(sbSockGetRcvChar(pChan->pSock) != '\r')
		return SR_RET_ERR;
	if(sbSockGetRcvChar(pChan->pSock) != '\n')
		return SR_RET_ERR;

	return SR_RET_OK;
}


/**
 * Helper to sbFramActualRecvFramAns and *Normal; receives the common
 * header off the wire.
 *
 * \param pThis [in/out] frame object to be filled. This is partly
 *                        created by the caller. new properties read
 *                        by this function are filled in. We do it
 *                        that way so that we can e.g. do error checking
 *                        for calloc() failures only in the caller. Saves
 *                        us some code ;)
 * \param pChan [in/out] the channel to receive the frame on.
 *                       The channel object is updated, e.g. with a
 *                       new seqno.
 */
srRetVal sbFramActualRecvFramCommonHdr(sbFramObj* pThis,sbChanObj *pChan)
{
	pThis->uMsgno = sbFramRecvUnsigned(pChan->pSock);

	if(sbSockGetRcvChar(pChan->pSock) != ' ')
		return SR_RET_ERR;
	pThis->cMore = sbSockGetRcvChar(pChan->pSock);

	if(sbSockGetRcvChar(pChan->pSock) != ' ')
		return SR_RET_ERR;
	pThis->uSeqno = sbFramRecvUnsigned(pChan->pSock);

	if(sbSockGetRcvChar(pChan->pSock) != ' ')
		return SR_RET_ERR;
	pThis->uSize = sbFramRecvUnsigned(pChan->pSock);

	/* We need to guard against oversized frames here.
	 * To do it fully right, we would need to check the
	 * remaining bytes in the window and compare this against
	 * uSize. However, we right now do it somewhat simpler and
	 * just compare against the max window size. Later releases
	 * can than add the more complex checks. This one here is
	 * at least good enough against malicous frames... )
	 */
	/** \todo improve! */
	if(pThis->uSize > BEEPFRAMEMAX)
		return SR_RET_OVERSIZED_FRAME;
	
	return SR_RET_OK;
}


/**
 * Helper to sbFramActualRecvFramAns and *Normal; receives the common
 * body off the wire. The body is defined to start WITH the CRLF in the
 * header. As such, this method processes the
 * 
 * - header CRLF
 * - payload
 * - trailer
 *
 * \param pThis [in/out] frame object to be filled. This is partly
 *                        created by the caller. new properties read
 *                        by this function are filled in. We do it
 *                        that way so that we can e.g. do error checking
 *                        for calloc() failures only in the caller. Saves
 *                        us some code ;)
 * \param pChan [in/out] the channel to receive the frame on.
 *                       The channel object is updated, e.g. with a
 *                       new seqno.
 */
sbFramActualRecvFramCommonBody(sbFramObj* pThis,sbChanObj *pChan)
{
	unsigned iToRcv;
	char szTrailer[6];
	char szPayload[BEEPFRAMEMAX+1];
	char *pszPayload = szPayload; /* one may argue if it is better to allocate this on the heap... */

	/* process rest of header (CRLF) */
	if(sbSockGetRcvChar(pChan->pSock) != '\r')
		return SR_RET_ERR;
	if(sbSockGetRcvChar(pChan->pSock) != '\n')
		return SR_RET_ERR;


	/* now process payload */
	for(iToRcv = pThis->uSize ; iToRcv > 0 ; --iToRcv)
	{
		int c;

		c = sbSockGetRcvChar(pChan->pSock);
		if(!c) /* this is not allowed as per spec - but what should we do... */
			c = ' ';

		*pszPayload++ = c;
	}
	*pszPayload = '\0';

	pThis->szRawBuf = malloc((pThis->uSize + 1) * sizeof(char));
	memcpy(pThis->szRawBuf, szPayload, (pThis->uSize + 1) * sizeof(char));
	pThis->iFrameLen = pThis->uSize;

	/* now process trailer ('E' 'N' 'D' CRLF)*/
	szTrailer[0] = sbSockGetRcvChar(pChan->pSock);
	szTrailer[1] = sbSockGetRcvChar(pChan->pSock);
	szTrailer[2] = sbSockGetRcvChar(pChan->pSock);
	szTrailer[3] = sbSockGetRcvChar(pChan->pSock);
	szTrailer[4] = sbSockGetRcvChar(pChan->pSock);
	szTrailer[5] = '\0';
	if(strcmp(szTrailer, "END\r\n"))
	{
		return SR_RET_ERR;
	}

	return SR_RET_OK;
}


/**
 * Helper to sbFramActualRecvFram; receives an
 * ANS frame off the wire.
 *
 * \param pThis [in/out] frame object to be filled. This is partly
 *                        created by the caller. new properties read
 *                        by this function are filled in. We do it
 *                        that way so that we can e.g. do error checking
 *                        for calloc() failures only in the caller. Saves
 *                        us some code ;)
 * \param pChan [in/out] the channel to receive the frame on.
 *                       The channel object is updated, e.g. with a
 *                       new seqno.
 */
srRetVal sbFramActualRecvFramANS(sbFramObj* pThis,sbChanObj *pChan)
{
	srRetVal iRet;

	/* first receive the "normal" stuff... */
	if((iRet = sbFramActualRecvFramCommonHdr(pThis, pChan)) != SR_RET_OK)
		return iRet;

	/* now go for ansno */
	if(sbSockGetRcvChar(pChan->pSock) != ' ')
		return SR_RET_ERR;
	pThis->uAnsno = sbFramRecvUnsigned(pChan->pSock);

	/* and now receive the rest of the frame... */
	return sbFramActualRecvFramCommonBody(pThis, pChan);
}

/**
 * Helper to sbFramActualRecvFram; receives a normal (that is
 * non-SEQ and non-ANS) frame off the wire.
 *
 * \param pThis [in/out] frame object to be filled. This is partly
 *                        created by the caller. new properties read
 *                        by this function are filled in. We do it
 *                        that way so that we can e.g. do error checking
 *                        for calloc() failures only in the caller. Saves
 *                        us some code ;)
 * \param pChan [in/out] the channel to receive the frame on.
 *                       The channel object is updated, e.g. with a
 *                       new seqno.
 */
srRetVal sbFramActualRecvFramNormal(sbFramObj* pThis,sbChanObj *pChan)
{
	srRetVal iRet;

	/* first receive the "normal" stuff... */
	if((iRet = sbFramActualRecvFramCommonHdr(pThis, pChan)) != SR_RET_OK)
		return iRet;

	/* and now receive the rest of the frame... */
	return sbFramActualRecvFramCommonBody(pThis, pChan);
}


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */


BEEPHdrID sbFramHdrID(char* szCmd)
{
	BEEPHdrID idHdr;

	if(!strcmp(szCmd, "SEQ"))
		idHdr = BEEPHDR_SEQ;
	else if(!strcmp(szCmd, "ANS"))
		idHdr = BEEPHDR_ANS;
	else if(!strcmp(szCmd, "MSG"))
		idHdr = BEEPHDR_MSG;
	else if(!strcmp(szCmd, "ERR"))
		idHdr = BEEPHDR_ERR;
	else if(!strcmp(szCmd, "NUL"))
		idHdr = BEEPHDR_NUL;
	else if(!strcmp(szCmd, "RPY"))
		idHdr = BEEPHDR_RPY;
	else
		idHdr = BEEPHDR_UNKNOWN;

	return(idHdr);
}


srRetVal sbFramConstruct(sbFramObj **ppThis)
{
	if((*ppThis = calloc(1, sizeof(sbFramObj))) == NULL)
	{
		return SR_RET_OUT_OF_MEMORY;
	}

	(*ppThis)->OID = OIDsbFram;
	(*ppThis)->idHdr = BEEPHDR_UNKNOWN;
#	if FEATURE_LISTENER == 1
		(*ppThis)->OnFramDestroy = NULL;
		(*ppThis)->pUsr = NULL;
#	endif

	return SR_RET_OK;
}


BEEPHdrID sbFramGetHdrID(sbFramObj *pThis)
{
	/* if the BEEP header ID is not known, it is
	 * constructed on the fly.
	 */
	assert(pThis != NULL);
	return(pThis->idHdr);
}

int sbFramGetFrameLen(sbFramObj* pThis)
{
	assert(pThis != NULL);
	return(pThis->iFrameLen);
}

char* sbFramGetFrame(sbFramObj* pThis)
{
	assert(pThis != NULL);
	return(pThis->szRawBuf);
}

sbFramObj* sbFramCreateFramFromMesg(sbChanObj* pChan, sbMesgObj* pMesg, char* pszCmd, SBansno uAnsno)
{
	sbFramObj *pThis;
	char *pszFramBuf;

	assert(pMesg != NULL);
	assert(pszCmd != NULL);
	assert(strlen(pszCmd) == 3);

	if((pThis = calloc(1, sizeof(sbFramObj))) == NULL)
	{
		return NULL;
	}

	if((pszFramBuf = malloc(sizeof(char) * BEEPFRAMEMAX + 1)) == NULL)
	{
		SRFREEOBJ(pThis);
		return NULL;
	}
	
	if(!strcmp(pszCmd, "ANS"))
	{
		SNPRINTF(pszFramBuf, BEEPFRAMEMAX + 1, "%3.3s %u %u . %u %u %u\r\n%sEND\r\n",
				pszCmd, pChan->uChanNum, pChan->uMsgno, pChan->uSeqno, sbMesgGetOverallSize(pMesg), uAnsno, sbMesgGetRawBuf(pMesg));
	}
	else
	{
		SNPRINTF(pszFramBuf, BEEPFRAMEMAX + 1, "%3.3s %u %u . %u %u\r\n%sEND\r\n",
				pszCmd, pChan->uChanNum, pChan->uMsgno, pChan->uSeqno, sbMesgGetOverallSize(pMesg), sbMesgGetRawBuf(pMesg));
	}

	/* debug:	printf("\n###########################Sending, Payload size %d#################\n%s#########END SEND######################################\n",
		    sbMesgGetOverallSize(pMesg), pszFramBuf);*/

	/* update channel object */
	pChan->uSeqno += sbMesgGetOverallSize(pMesg);
	pChan->uMsgno++;

	pThis->iFrameLen = (int) strlen(pszFramBuf);
	pThis->uSize = sbMesgGetOverallSize(pMesg);
	pThis->szRawBuf = pszFramBuf;
	pThis->OID = OIDsbFram;
	pThis->idHdr = sbFramHdrID(pszCmd);
	/* We do NOT allocate a specifcally sized copy of the
	 * message buffer. OK, we use some more memory right now
	 * then we needed to do, but that buys us some performance.
	 * I think this is better than copying everything over to a
	 * new buffer (it will be free()ed soon, anyhow...).
	 */
	pThis->iState = sbFRAMSTATE_READY_TO_SEND;
    
	return pThis;
}


srRetVal sbFramCreateSEQFram(sbFramObj **ppThis, sbChanObj* pChan, SBackno uAckno, SBwindow uWindow)
{
	char *pszFramBuf;

	sbChanCHECKVALIDOBJECT(pChan);
	assert(ppThis != NULL);
	
	if(uAckno == 0)
		return SR_RET_ACKNO_ZERO;

	if(uWindow == 0)
		uWindow = BEEP_DEFAULT_WINDOWSIZE;

	if((*ppThis = calloc(1, sizeof(sbFramObj))) == NULL)
	{
		return SR_RET_OUT_OF_MEMORY;
	}

	if((pszFramBuf = malloc(sizeof(char) * 64)) == NULL)
	{	/* 64 is sufficiently large */
		SRFREEOBJ(*ppThis);
		return SR_RET_OUT_OF_MEMORY;
	}

	SNPRINTF(pszFramBuf, 64, "SEQ %u %u %u\r\n",
			 pChan->uChanNum, uAckno, uWindow);

	(*ppThis)->uSize = (*ppThis)->iFrameLen = (int) strlen(pszFramBuf);
	(*ppThis)->szRawBuf = pszFramBuf;
	(*ppThis)->OID = OIDsbFram;
	(*ppThis)->idHdr = BEEPHDR_SEQ;
	(*ppThis)->iState = sbFRAMSTATE_READY_TO_SEND;
    
	return SR_RET_OK;
}

sbFramObj* sbFramActualRecvFram(sbSessObj *pSess)
{
	sbFramObj *pThis;
	SBchannel uChan;
	char szCmd[4];
	sbChanObj* pChan;
	srRetVal (*pRecvFramFunc)(sbFramObj*, sbChanObj*);
	BEEPHdrID idHdr;
	
	sbSessCHECKVALIDOBJECT(pSess);

	/* We now receive field by field of the header */

	/* HDR (command)
	 * allowed headers in this release are:
	 * ANS
	 * ERR
     * MSG
	 * NUL
	 * RPY
	 * SEQ
	 *
	 * As such, we can only have letters A,E,M,N,R,S on the first byte. We 
	 * check this to ensure we have a valid packet. Of course, we also need
	 * to check the second and third byte ;)
	 */
	szCmd[0] = sbSockGetRcvChar(pSess->pSock);
	if(szCmd[0] != 'A' && szCmd[0] != 'E' && szCmd[0] != 'M'  && szCmd[0] != 'N' && szCmd[0] != 'R' && szCmd[0] != 'S')
		return(NULL);

	szCmd[1] = sbSockGetRcvChar(pSess->pSock);
	/* The following check still allows some invalid cases. However, we have decided
	 * that this is good enough, especially as after the next char a full verification
	 * is done and an error would be caught then. The current solution has advantages from a
	 * performance point of view.
	 */
	if(szCmd[1] != 'N' && szCmd[1] != 'R' && szCmd[1] != 'S'  && szCmd[1] != 'U' && szCmd[1] != 'P' && szCmd[1] != 'E')
		return(NULL);

	szCmd[2] = sbSockGetRcvChar(pSess->pSock);
	szCmd[3] = '\0';

	/* now we have to complete header. The name is checked and
	 * the actual handler is associated.
	 */

	idHdr = sbFramHdrID(szCmd);

	if(idHdr == BEEPHDR_UNKNOWN)
		return NULL;
	else if(idHdr == BEEPHDR_SEQ)
		pRecvFramFunc = sbFramActualRecvFramSEQ;
	else if(idHdr == BEEPHDR_ANS)
		pRecvFramFunc = sbFramActualRecvFramANS;
	else
		pRecvFramFunc = sbFramActualRecvFramNormal;

	if(sbSockGetRcvChar(pSess->pSock) != ' ')
	{
		return NULL;
	}
	uChan = sbFramRecvUnsigned(pSess->pSock);

	/* "eat" next SP char as this must be done in any case... */
	if(sbSockGetRcvChar(pSess->pSock) != ' ')
	{
		return NULL;
	}

	if((pChan = sbSessRetrChanObj(pSess, uChan)) == NULL)
	{
		return NULL;
	}

	/* OK, we now know that it looks like a valid frame within a 
	 * valid channel. So it is now time to create the frame object
	 * and pass it down to further parsing.
	 */
	if((pThis = calloc(1, sizeof(sbFramObj))) == NULL)
		return NULL;

	pThis->idHdr = idHdr;
	pThis->OID = OIDsbFram;
	pThis->uChannel = uChan;
	pThis->iState = sbFRAMSTATE_RECEIVED;

	if((*pRecvFramFunc)(pThis, pChan) != SR_RET_OK)
	{
		free(pThis);
		return NULL;
	}
	return(pThis);
}



srRetVal sbFramDestroy(sbFramObj *pThis)
{
	sbFramCHECKVALIDOBJECT(pThis);

	if(pThis->szRawBuf != NULL)
		free(pThis->szRawBuf);

#	if FEATURE_LISTENER == 1
	/* please note: the call may destroy the channel. As such,
	 * no access the the channel object is allowed after
	 * OnFramDestroy() has been called!
	 */
		if(pThis->OnFramDestroy != NULL)
			pThis->OnFramDestroy(pThis);
#	endif

	SRFREEOBJ(pThis);

	return SR_RET_OK;
}


sbFramObj* sbFramRecvFram(sbChanObj* pChan)
{
	return sbSessRecvFram(pChan->pSess, pChan);
}



srRetVal sbFramSendFram(sbFramObj* pThis, sbChanObj*pChan)
{
	sbFramCHECKVALIDOBJECT(pThis);
	sbChanCHECKVALIDOBJECT(pChan);


	return (pChan->pSess->SendFramMethod)(pChan->pSess, pThis, pChan);
}

/* ######################### LISTENER ONLY ############################### */
#if FEATURE_LISTENER == 1

srRetVal sbFramSetOnDestroyEvent(sbFramObj *pThis, void (*OnFramDestroy)(sbFramObj*), void *pUsr)
{
	sbFramCHECKVALIDOBJECT(pThis);
	assert(OnFramDestroy != NULL);

	pThis->OnFramDestroy = OnFramDestroy;
	pThis->pUsr = pUsr;

	return SR_RET_OK;
}

#endif
/* ####################### END LISTENER ONLY ############################# */
