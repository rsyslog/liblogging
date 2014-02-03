/*! \file beepmessage.c
 *  \brief Implementation of the BEEP Message Object.
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
#include "settings.h"
#include "liblogging.h"
#include "beepmessage.h"
#include "beepframe.h"

/* ################################################################# *
 * private members                                                   *
 * ################################################################# */


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */

char* sbMesgGetRawBuf(sbMesgObj* pThis)
{
	assert(pThis != NULL);
	return(pThis->szRawBuf);
}

int sbMesgGetMIMEHdrSize(sbMesgObj* pThis)
{
	assert(pThis != NULL);
	return(pThis->iMIMEHdrSize);
}

int sbMesgGetPayloadSize(sbMesgObj* pThis)
{
	assert(pThis != NULL);
	return(pThis->iPayloadSize);
}

int sbMesgGetOverallSize(sbMesgObj* pThis)
{
	assert(pThis != NULL);
	return(pThis->iOverallSize);
}

sbMesgObj* sbMesgConstruct(char* pszMIMEHdr, char *pszPayload)
{
	sbMesgObj *pThis;
	char* pszMsgBuf;
	char* pszHdrBuf;
	int iHdrSize, iPayloadSize, iOverallSize;

	if((pThis = calloc(1, sizeof(sbMesgObj))) == NULL)
		return NULL;

	/* compute sizes */
	iHdrSize = (pszMIMEHdr == NULL) ? 0 : (int) strlen(pszMIMEHdr);
	iPayloadSize = (pszPayload == NULL) ? 0 : (int) strlen(pszPayload);
	iOverallSize = iHdrSize + 2 /* CRLF */ + iPayloadSize;
	if((pszMsgBuf = malloc((iOverallSize + 1/* \0 byte*/) * sizeof(char))) == NULL)
	{	/* oops, doesn't work out... */
		SRFREEOBJ(pThis);	/* no further need */
		return(NULL);
	}
	if((pszMIMEHdr != NULL) && (pszHdrBuf = malloc((iHdrSize + 1 /*\0!*/) * sizeof(char))) == NULL)
	{	/* oops, doesn't work out... */
		free(pszMsgBuf);
		SRFREEOBJ(pThis);	/* no further need */
		return(NULL);
	}

	/* If we reach this point, we can finally construct the object. All
		* memory allocations have succeeded.
		*/
	pThis->szRawBuf = pszMsgBuf;
	pThis->OID = OIDsbMesg;
	if(pszMIMEHdr == NULL)
	{
		pThis->szMIMEHdr = NULL;
	}
	else
	{
		strcpy(pszHdrBuf, pszMIMEHdr);
		pThis->szMIMEHdr = pszHdrBuf;
		strcpy(pszMsgBuf, pszMIMEHdr);
		pszMsgBuf += iHdrSize;
	}

	*pszMsgBuf++ = 0x0d;
	*pszMsgBuf++ = 0x0a;
	if(pszPayload != NULL)
	{
		memcpy(pszMsgBuf, pszPayload, iPayloadSize + 1);
	}
	else
	{ /* \0 terminator must be written in this case! */
		pszMsgBuf = '\0';
	}
	/* finish initialization */
	pThis->szActualPayload = pThis->szRawBuf + iHdrSize + 2;
	pThis->iMIMEHdrSize = iHdrSize;
	pThis->iPayloadSize = iPayloadSize;
	pThis->iOverallSize = iOverallSize;

	/* debug: printf("Mesg# RawBuf: alloc %d, strlen() %d - HDRBuf: alloc %d, strlen() %d.\n", iOverallSize + 1, strlen(pThis->szRawBuf), iHdrSize + 1, (pThis->szMIMEHdr == NULL) ? 0 : strlen(pThis->szMIMEHdr)); */
	return(pThis);
}


srRetVal sbMIMEExtract(char *pszInBuf, int iInBufLen, char **pszMIMEHdr, char** pszPayload)
{
	char* pHdr;
	char* pPayload = NULL;
	char* psz;
	int iCurrCol = 0;
	int iHdrLen;
	int iPayloadLen;

	/* detect end of MIME Header */
	/* we now handle empty strings, but this fix needs further testing. I just
	 * added the "if(*psz)" and now I wonder why this wasn't done in the first
	 * place. However, this was 3 years ago and my memory has vanished... -- rger, 2006-10-06 */
	if(*pszInBuf)
	{
		for(psz = pszInBuf ; *(psz+1) != '\0' ; ++psz)
		{
			if(*psz == '\r' && *(psz+1) == '\n')
			{
				if(iCurrCol == 0)
				{
					pPayload = psz + 2;
					break;
				}
				else
				{
					iCurrCol = 0;
					++psz;
				}
			}
			else
				++iCurrCol;
		}
	}

	if(pPayload == NULL)	/* MIME Header missing? */
	{
		/* We need to allow this for SDCS for the time being. We also
		 * need it to guard us against malicious traffic... So it must
		 * stay in here even when SDSC is fixed. If we do not find any
		 * MIME header (not even an empty one), we assume there was an
		 * empty one.
		 */
		pHdr = NULL;
		pPayload = pszInBuf;
		iHdrLen = 0;
		iPayloadLen = iInBufLen + 1 /* '\0' */;
	}
	else
	{
		iHdrLen = (int) (pPayload - pszInBuf - 2 /* CRLF */);
		iPayloadLen = iInBufLen - (iHdrLen + 2 /*CRLF */) + 1 /* \0 */;
	}

	/* if we reach this, we have successfully parsed the header
	 * and can now create the buffers.
	 */
	if(iHdrLen == 0)
	{
		*pszMIMEHdr = NULL;
	}
	else
	{
		if((*pszMIMEHdr = malloc((iHdrLen + 1) * sizeof(char))) == NULL)
		{
			*pszPayload = NULL; /* just in case... */
			return SR_RET_OUT_OF_MEMORY;
		}
		memcpy(*pszMIMEHdr, pszInBuf, iHdrLen);
		*(*pszMIMEHdr+iHdrLen) = '\0'; /* no additional +1 needed as +IhdrLen is already after the string */
	}

	if((*pszPayload = malloc(iPayloadLen * sizeof(char))) == NULL)
	{
		free(*pszMIMEHdr);
		*pszMIMEHdr = NULL;
		return SR_RET_OUT_OF_MEMORY;
	}
	strcpy(*pszPayload, pPayload);

	/* debug:	printf("sbMIMEExtract# HDRBuf: alloc %d, strlen() %d - BodyBuf: alloc %d, strlen() %d.\n", iHdrLen + 1, (*pszMIMEHdr == NULL) ? 0 : strlen(*pszMIMEHdr), iInBufLen - (iHdrLen + 2 ) + 1, strlen(*pszPayload)); */

	return SR_RET_OK;
}


sbMesgObj* sbMesgRecvMesg(sbChanObj* pChan)
{
	sbFramObj *pFram;
	sbMesgObj *sbMesg;

	if((pFram = sbFramRecvFram(pChan)) == NULL)
		return NULL;

	sbMesg = sbMesgConstrFromFrame(pFram);

	sbFramDestroy(pFram);

	return(sbMesg);
}

sbMesgObj* sbMesgConstrFromFrame(sbFramObj* psbFram)
{
	sbMesgObj *pThis;
	char* pszMIMEHdr;
	char* pszPayload;

	/* first extract Header and Payload */
	if(sbMIMEExtract(sbFramGetFrame(psbFram), sbFramGetFrameLen(psbFram), &pszMIMEHdr, &pszPayload) != SR_RET_OK)
		return NULL;

	/* then use the "normal" constructor... */

	pThis = sbMesgConstruct(pszMIMEHdr, pszPayload);
	free(pszMIMEHdr);
	free(pszPayload);
	pThis->idHdr = psbFram->idHdr;
	pThis->uMsgno = psbFram->uMsgno;
	pThis->uSeqno = psbFram->uSeqno;
	pThis->uNxtSeqno = psbFram->uSeqno + psbFram->uSize;

	return(pThis);
}


srRetVal sbMesgSendMesgWithCallback(sbMesgObj* pThis, sbChanObj* pChan, char *pszCmd, SBansno uAnsno,  void (*OnFramDestroy)(sbFramObj*), void* pUsr)
{
	sbFramObj* pFram;
	srRetVal iRet;

	sbMesgCHECKVALIDOBJECT(pThis);
	sbChanCHECKVALIDOBJECT(pChan);

	if((pFram = sbFramCreateFramFromMesg(pChan, pThis, pszCmd, uAnsno)) == NULL)
		return SR_RET_ERR;

	if(OnFramDestroy != NULL)
		if((iRet = sbFramSetOnDestroyEvent(pFram, OnFramDestroy, pUsr)) != SR_RET_OK)
			return iRet;

	pThis->idHdr = pFram->idHdr;
	pThis->uMsgno = pFram->uMsgno;

	/**
	 * For now, we assume a Mesg will always fit into one Fram. This
	 * is always true for syslog. So there is no need to do it more
	 * complicated. In the long term, it may be worth changing this...
	 */

	iRet = sbFramSendFram(pFram, pChan);
	if(pFram->iState == sbFRAMSTATE_SENT)
		sbFramDestroy(pFram);

	return(iRet);
}

srRetVal sbMesgSendMesg(sbMesgObj* pThis, sbChanObj* pChan, char *pszCmd, SBansno uAnsno)
{
	return(sbMesgSendMesgWithCallback(pThis, pChan, pszCmd, uAnsno,  NULL, NULL));
}


void sbMesgDestroy(sbMesgObj *pThis)
{
	sbMesgCHECKVALIDOBJECT(pThis);

	if(pThis->szRawBuf != NULL)
		free(pThis->szRawBuf);
	if(pThis->szMIMEHdr != NULL)
		free(pThis->szMIMEHdr);
	SRFREEOBJ(pThis);
}
