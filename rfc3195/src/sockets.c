/**\file sockets.c
 * \brief Implements the socket object.
 *
 * This file implements the socket layer. It contains all
 * those methods that are upper layer. It also includes
 * lower layer files which do the actual OS-dependant socket API
 * handling. Thanks to Devin Kowatch, there is now a UNIX as well
 * as a Win32 lower layer available.
 *
 * \note If you intend to add a new lower layer, please
 * - create a new file socketsXXX.c, where XXX is the 
 *   OS dependant name.
 * - update the conditional compilation at the end of
 *   this file to include your new lower layer.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-05
 *          Created this file as upper-layer and general entry point
 *          to the socket object. It includes the necessry lower
 *          layer according to the settings.h OS defines.
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

#include "settings.h"
#include "liblogging.h"
#include "sockets.h"
#include "assert.h"

/* Now include the lower level driver so that we have
 * private (static) methods available during the rest of
 * the file.
 *
 * We also define some prototpyes of private
 * members that follow later in this file (no way
 * around this...).
 */
static int sbSockSetSockErrState(struct sbSockObject *pThis);

#ifdef SROS_WIN32
#	include "socketsWin32.c"
#else
#	include "socketsUnix.c"
#endif


/* ################################################################# *
 * private members                                                   *
 * ################################################################# */

/** Set error state and record socket error. Call this
 *  after an socket error has occured. Please note that
 *  in later releases it may be enhanced to call an
 *  upper-layer provided error handler (if the upper
 *  layer asked for this).
 *
 *  \retval liblogging return code for common socket errors.
 */
static srRetVal sbSockSetSockErrState(struct sbSockObject *pThis)
{
	pThis->bIsInError = TRUE;
	return sbSockSetLastSockError(pThis);
}


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */


/**
 * Construct a socket object (just in-memory 
 * representation, no socket calls).
 */
srRetVal sbSockConstruct(sbSockObj** pThis)
{
	if((*pThis = (struct sbSockObject*) calloc(1, sizeof(struct sbSockObject))) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	/* initialize class members */
	(*pThis)->sock = INVALID_SOCKET;
	(*pThis)->bIsInError = FALSE;
	(*pThis)->OID = OIDsbSock;
	(*pThis)->iCurInBufPos = 0;
	(*pThis)->iInBufLen = 0;
	(*pThis)->pRemoteHostIP = NULL;

	return SR_RET_OK;
}


int sbSockGetLastSockError(sbSockObj* pThis)
{
	sbSockCHECKVALIDOBJECT(pThis);
	return(pThis->dwLastError);
}


int sbSockPeekRcvChar(sbSockObj *pThis)
{
	sbSockCHECKVALIDOBJECT(pThis);

	if(pThis->iCurInBufPos >= pThis->iInBufLen)
	{	/* ok, we ate up the buffer. Need to re-read.
		 * we assume that there must be data as of the protocol,
		 * so at least in this version of the lib we block and
		 * wait.
		 *
		 * \todo check for errors!
		 */
		if((pThis->iInBufLen = sbSockReceive(pThis, pThis->szInBuf, SOCKETMAXINBUFSIZE)) < 0)
			return sbSOCK_NO_CHAR;
		pThis->iCurInBufPos = 0;
	}

	/* we now have at least one char in the buffer */
	return((pThis->iCurInBufPos >= pThis->iInBufLen) ? sbSOCK_NO_CHAR : pThis->szInBuf[pThis->iCurInBufPos]);
}


int sbSockGetRcvChar(sbSockObj *pThis)
{
	sbSockCHECKVALIDOBJECT(pThis);

	if(pThis->iCurInBufPos >= pThis->iInBufLen)
	{	/* ok, we ate up the buffer. Need to re-read.
		 * we assume that there must be data as of the protocol,
		 * so at least in this version of the lib we block and
		 * wait.
		 */
		if((pThis->iInBufLen = sbSockReceive(pThis, pThis->szInBuf, SOCKETMAXINBUFSIZE)) < 0)
			return sbSOCK_NO_CHAR;
		pThis->iCurInBufPos = 0;
	}

	/* we now have at least one char in the buffer */
	return((pThis->iCurInBufPos >= pThis->iInBufLen) ? sbSOCK_NO_CHAR : pThis->szInBuf[pThis->iCurInBufPos++]);
}


srRetVal sbSockExit(struct sbSockObject *pThis)
{
	srRetVal iRetVal = SR_RET_OK;

	sbSockCHECKVALIDOBJECT(pThis);

	if(pThis->sock != INVALID_SOCKET)
	{
		iRetVal = sbSockClosesocket(pThis);
		/* OK, we may have failed, but at this stage we still continue
		 * our processing - otherwise we may have an additional memory
		 * leak!
		 */
	}

	if(pThis->pRemoteHostIP != NULL)
		free(pThis->pRemoteHostIP);
	SRFREEOBJ(pThis);	/* there is nothing we can do if free() fails... */

	return(iRetVal);
}


int sbSockHasReceiveData(struct sbSockObject* pThis)
{
	int iRet;

	sbSockCHECKVALIDOBJECT(pThis);
	/* first check, if there is still data left in the in-memory buffer */
	if(pThis->iCurInBufPos < pThis->iInBufLen)
		return TRUE;

	iRet = sbSockSelect(pThis, 0, 0);

	if(iRet < 0)
		sbSockSetSockErrState(pThis);

	return((iRet == 1) ? TRUE : FALSE);
}


srRetVal sbSockWaitReceiveData(struct sbSockObject* pThis)
{
	int iRet;

	sbSockCHECKVALIDOBJECT(pThis);
	iRet = sbSockSelect(pThis, -1, -1);

	if(iRet < 0)
	{
		sbSockSetSockErrState(pThis);
		return SR_RET_ERR;
	}

	return(SR_RET_OK);
}

sbSockObj* 	sbSockInitListenSock(srRetVal *iRet, int iType, char *szBindToAddress, unsigned uBindToPort)
{
	sbSockObj* pThis;

	if((pThis = sbSockInitEx(AF_INET, iType)) == NULL)
	{
		*iRet = SR_RET_ERR;
		return NULL;
	}

	if((*iRet = sbSockBind(pThis, szBindToAddress, uBindToPort)) != SR_RET_OK)
		return NULL;

	return pThis;
}


srRetVal sbSockAcceptConnection(sbSockObj* pThis, sbSockObj** pNew)
{
	srRetVal iRet;
	struct sockaddr_in sa;
	int iLenSA;

	sbSockCHECKVALIDOBJECT(pThis);
	assert(pNew != NULL);

	if((iRet = sbSockConstruct(pNew)) != SR_RET_OK)
		return iRet;

	/* for now, we do not deliver back the address of the remote host */
	iLenSA = sizeof(struct sockaddr_in);
	if((iRet = sbSockAccept(pThis, *pNew, (struct sockaddr*) &sa, &iLenSA)) != SR_RET_OK)
	{
		sbSockExit(*pNew);
		return iRet;
	}

	memcpy(&((*pNew)->RemoteHostAddr), &sa, sizeof(struct sockaddr_in));

	return SR_RET_OK;
}


srRetVal sbSockGetRemoteHostIP(sbSockObj *pThis, char **ppszHost)
{
	char *pBuf;
	char *pBufRTL;
	srRetVal iRet;
	sbSockCHECKVALIDOBJECT(pThis);
	assert(ppszHost != NULL);

	if(pThis->pRemoteHostIP == NULL)
	{	/* need to obtain the string */
		if((iRet = sbSock_inet_ntoa(&(pThis->RemoteHostAddr), &pBufRTL)) != SR_RET_OK)
			return iRet;
		
		/* we must now copy the returned buffer, as it is owned by the run time library (see man) */
		pThis->iRemHostIPBufLen = (int) strlen(pBufRTL) + 1;
		if((pThis->pRemoteHostIP = (char*) malloc(pThis->iRemHostIPBufLen * sizeof(char))) == NULL)
			return SR_RET_OUT_OF_MEMORY;
		memcpy(pThis->pRemoteHostIP, pBufRTL, pThis->iRemHostIPBufLen);
	}

	if((pBuf = (char*) malloc(pThis->iRemHostIPBufLen * sizeof(char))) == NULL)
		return SR_RET_OUT_OF_MEMORY;
	memcpy(pBuf, pThis->pRemoteHostIP, pThis->iRemHostIPBufLen);
	*ppszHost = pBuf;

	return SR_RET_OK;
}


srRetVal sbSockGetIPusedForSending(sbSockObj* pThis, char**ppsz)
{
	srRetVal iRet;
	struct sockaddr_in sa;
	int iLenSA;
	char *pBufRTL;
	int iLenBuf;

	sbSockCHECKVALIDOBJECT(pThis);
	assert(ppsz != NULL);
	assert(pThis->sock != INVALID_SOCKET);

	iLenSA = sizeof(sa);
	if((iRet = sbSock_getsockname(pThis, &sa, &iLenSA)) != SR_RET_OK)
		return iRet;

	/* got the address, let's format a string */
	if((iRet = sbSock_inet_ntoa(&sa, &pBufRTL)) != SR_RET_OK)
		return iRet;
		
	/* we must now copy the returned buffer, as it is owned by the run time library (see man) */
	iLenBuf = (int) strlen(pBufRTL) + 1;
	if((*ppsz = (char*) malloc(iLenBuf * sizeof(char))) == NULL)
		return SR_RET_OUT_OF_MEMORY;
	memcpy(*ppsz, pBufRTL, iLenBuf);

	return SR_RET_OK;
}

/**
 * Enhanced recvfrom() clone. Will receive an C sz String. That is, any
 * \0 received as part of the string will be replaced by ABNF SP (' ').
 *
 * \param pRecvBuf Pointer to buffer that will receive the incoming data.
 * \param piBufLen On entry, the size of the buffer pointed to by pRecvBuf.
 *                 On exit, the number of bytes received
 * \param ppFrom   Pointer to a char pointer that will receive a string with the
 *                 senders' IP address. This buffer MUST be free()ed by the caller!
 */
srRetVal sbSockRecvFrom(sbSockObj *pThis, char* pRecvBuf, int *piBufLen, char **ppFrom)
{
	srRetVal iRet;
	struct sockaddr_in sa;
	int iLenSA;
	int iLenStr;
	char *pBufRTL;

	sbSockCHECKVALIDOBJECT(pThis);
	assert(pThis->sock != INVALID_SOCKET);
	assert(pRecvBuf != NULL);
	assert(piBufLen != NULL);
	assert(*piBufLen > 0);

	iLenSA = sizeof(sa);
	*piBufLen = sbSock_recvfrom(pThis, pRecvBuf, (*piBufLen) - 1, 0, (struct sockaddr*) &sa, &iLenSA);

	if((iRet = sbSock_inet_ntoa(&sa, &pBufRTL)) != SR_RET_OK)
		return iRet;
	
	/* we must now copy the returned buffer, as it is owned by the run time library (see man) */
	pThis->iRemHostIPBufLen = (int) strlen(pBufRTL) + 1;
	if((*ppFrom = (char*) malloc(pThis->iRemHostIPBufLen * sizeof(char))) == NULL)
		return SR_RET_OUT_OF_MEMORY;
	memcpy(*ppFrom, pBufRTL, pThis->iRemHostIPBufLen);

	/** \todo handle the connection closed case (*piBuflen == 0) */
	if(*piBufLen >= 0)
		*(pRecvBuf + *piBufLen) = '\0';

	if(*piBufLen < 0)
		return SR_RET_ERR;

	/* now guard against \0 bytes */
	iLenStr = *piBufLen;
	while(iLenStr > 0)
	{
		if(*pRecvBuf == '\0')
			*pRecvBuf = ' ';
		++pRecvBuf;
		--iLenStr;
	}

	return SR_RET_OK;
}
