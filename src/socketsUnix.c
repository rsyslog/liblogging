/**\file socketsUnix.c
 * \brief Socket layer/driver for Unix.
 *
 * This is the low-level socket layer for Unix. This
 * file is included from \ref sockets.c.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \author  Devin Kowatch <devink@sdsc.edu>
 *
 * \date    2003-08-04
 *          rgerhards: inital version created
 *
 * \date    2003-08-04
 *          devink: initial UNIX port by 
 *
 * \date	2003-08-05
 *          rgerhards: Changed to be a lower layer to the generic sockets.c.
 * 
 * \date	2003-08-05
 *          devink: applied changes to compile under Solaris. AIX also works well.
 *
 * Copyright 2002-2003 
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
 *     * Neither the name of Adiscon GmbH or Rainer Gerhards
 *       nor the names of its contributors may be used to
 *       endorse or promote products derived from this software without
 *       specific prior written permission.
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

#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>



/* ################################################################# */
/* private members                                                   */
/* ################################################################# */


/* ################################################################# */
/* public members                                                    */
/* ################################################################# */

srRetVal sbSockLayerInit(int bInitOSStack)
{
	return SR_RET_OK;
}

srRetVal sbSockLayerExit(int bExitOSStack)
{
	return SR_RET_OK;
}

struct sbSockObject* sbSockInit(void)
{
	struct sbSockObject *pThis;

	pThis = (struct sbSockObject*) calloc(1, sizeof(struct sbSockObject));
	if(pThis != NULL)
	{	/* initialize class members */
		if((pThis->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			free(pThis);
			return(NULL);
		}
		/* rest of initialization */
		pThis->bIsInError = FALSE;
		pThis->OID = OIDsbSock;
		pThis->iCurInBufPos = 0;
		pThis->iInBufLen = 0;
	}
	return(pThis);
}


srRetVal sbSockClosesocket(sbSockObj *pThis)
{
	sbSockCHECKVALIDOBJECT(pThis);

	if(close(pThis->sock) < 0)
	{
		sbSockSetSockErrState(pThis);
		return SR_RET_ERR;
	}

	return SR_RET_OK;
}


/* ############################################################ */
/* # Now come more or less winsock wrappers                   # */
/* ############################################################ */

int sbSockSelect(struct sbSockObject* pThis, int iTimOutSecs, int iTimOutMSecs)
{
	int iRet;
	int iHighestDesc;
	fd_set fdset;
	struct timeval tv /*= {iTimOutSecs, iTimOutMSecs}*/;
	struct timeval *ptv;

	if(iTimOutSecs == -1)
		ptv = NULL;
	else
	{
		tv.tv_sec = iTimOutSecs;
		tv.tv_usec = iTimOutMSecs;
		ptv = &tv;
	}

	sbSockCHECKVALIDOBJECT(pThis);
	FD_ZERO(&fdset);
	FD_SET(pThis->sock, &fdset);
	iHighestDesc = pThis->sock + 1;
	iRet = select(iHighestDesc, &fdset, NULL, NULL, &tv);

	return(iRet);
}

int sbSockSelectMulti(srSock_fd_set *fdsetRD, srSock_fd_set *fdsetWR, int iTimOutSecs, int iTimOutMSecs, int iHighestDesc)
{
	int iRet;
	struct timeval tv;
	struct timeval *ptv;

	if(iTimOutSecs == -1)
		ptv = NULL;
	else
	{
		tv.tv_sec = iTimOutSecs;
		tv.tv_usec = iTimOutMSecs;
		ptv = &tv;
	}
		
	iHighestDesc++; /* must be plus 1 according to socket API! */
	iRet = select(iHighestDesc, fdsetRD, fdsetWR, NULL, ptv);

	return(iRet);
}


int sbSockReceive(struct sbSockObject* pThis, char * pszBuf, int iLen)
{
	int	iBytesRcvd;
	sbSockCHECKVALIDOBJECT(pThis);

	/* iLen - 1 to leave room for the terminating \0 character */
	iBytesRcvd = recv(pThis->sock, pszBuf, iLen - 1, 0);

	if(iBytesRcvd < 0)
	{
		sbSockSetSockErrState(pThis);
		*pszBuf = '\0';	/* Just in case... */
	}
	else
	{	/* terminate string! */
		*(pszBuf + iBytesRcvd) = '\0';
	}

	return(iBytesRcvd);
}


int sbSockSend(struct sbSockObject* pThis, const char* pszBuf, int iLen)
{
	int	iRetCode = -1;
	sbSockCHECKVALIDOBJECT(pThis);

	if(pszBuf != NULL)
	{
		iRetCode = send(pThis->sock, pszBuf, iLen, 0);
		if(iRetCode < 0)
		{
			sbSockSetSockErrState(pThis);
			iRetCode = -1;
		}
	}

	return(iRetCode);
}

srRetVal sbSockConnectoToHost(sbSockObj* pThis, char* pszHost, int iPort)
{
	struct sockaddr_in remoteaddr;
	int	remoteaddrlen = sizeof(remoteaddr);
	struct hostent *hstent;
	struct sockaddr_in srv_addr;
	char* pIpAddr;
	int iRetCode;

	sbSockCHECKVALIDOBJECT(pThis);

	/* Bind socket to any port on local machine */
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = INADDR_ANY;
	srv_addr.sin_port = 0;

	if(bind(pThis->sock, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0)
	{
		/* debug: fprintf(stderr, "Error binding socket!\r\n"); */
		return SR_RET_ERR;
	}

	/* Create target address */
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_port = htons(iPort);
	
	hstent = gethostbyname(pszHost);

	if(hstent == NULL)
	{
		sbSockSetSockErrState(pThis);
		return SR_RET_ERR;
	}

	/* extract host address - we always use the first one */
	pIpAddr = *hstent->h_addr_list;

    memcpy(&(remoteaddr.sin_addr.s_addr), pIpAddr, sizeof(remoteaddr.sin_addr.s_addr));

	iRetCode = connect(pThis->sock, (struct sockaddr*) &remoteaddr, sizeof(remoteaddr));

	if(iRetCode < 0)
	{
		sbSockSetSockErrState(pThis);
		return SR_RET_ERR;
	}

	return SR_RET_OK;
}

srRetVal sbSockAccept(sbSockObj*pThis, sbSockObj* pNew, struct sockaddr *sa, int *iSizeSA)
{
	sbSockCHECKVALIDOBJECT(pThis);
	sbSockCHECKVALIDOBJECT(pNew);

	if((pNew->sock = accept(pThis->sock, sa, iSizeSA)) == INVALID_SOCKET)
		return sbSockSetSockErrState(pThis);

	return SR_RET_OK;
}


srRetVal sbSockListen(sbSockObj*pThis)
{
	int	iRetCode;
	sbSockCHECKVALIDOBJECT(pThis);

	if((iRetCode = listen(pThis->sock, SOMAXCONN)) != 0)
	{
		sbSockSetSockErrState(pThis);
		return SR_RET_ERR;
	}

	return SR_RET_OK;
}


/* rgerhards: I have coded this not really knowing what I 
 * am doing. Some Unix guy should have a look at this ;)
 */
srRetVal sbSockSetNonblocking(sbSockObj*pThis)
{
	sbSockCHECKVALIDOBJECT(pThis);

	if(fcntl(pThis->sock, F_SETFL,  O_NONBLOCK) == -1)
		return sbSockSetSockErrState(pThis);

	return SR_RET_OK;
}

/* rgerhards: I have coded this not really knowing what I 
 * am doing. Some Unix guy should have a look at this ;)
 */
static int sbSockSetLastSockError(struct sbSockObject *pThis)
{
	srRetVal iRet;
	pThis->dwLastError = errno;

	switch(pThis->dwLastError)
	{
	case EINVAL:
		iRet = SR_RET_INVALID_SOCKET;
		break;
	default:
		iRet = SR_RET_SOCKET_ERR;
		break;
	}

	return iRet;
}


srRetVal sbSockBind(sbSockObj* pThis, char* pszHost, int iPort)
{
	struct sockaddr_in srv_addr;

	sbSockCHECKVALIDOBJECT(pThis);
	assert(pszHost == NULL);	/** sorry, other modes currently not supported \todo implement! */

	/* Bind socket to any port on local machine */
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = INADDR_ANY;
	srv_addr.sin_port = htons(iPort);

	if(bind(pThis->sock, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0)
	{
		perror("socket bind");
		return SR_RET_CANT_BIND_SOCKET;
	}

	return SR_RET_OK;
}


/** 
 * Wrapper for inet_ntoa().
 *
 * \param psa Poiner to a struct sockaddr for which the representation
 *            is to obtain. Must not be NULL.
 *
 * \param psz Pointer to buffer that should receive the IP address
 * string pointer. Must not be NULL. NOTE WELL: This pointer is
 * only valid until the next socket call. So the caller should 
 * immediately copy it to its own buffer once control returns.
 * The returned buffer MUST NOT be free()ed.
 */
static srRetVal sbSock_inet_ntoa(struct sockaddr_in *psa, char **psz)
{
	assert(psz != NULL);

	if((*psz = (char*) inet_ntoa(psa->sin_addr)) == NULL)
		return SR_RET_ERR;

	return SR_RET_OK;
}


/**
 * Wrapper for gethostname().
 *
 * \param psz Pointer to Pointer to hostname. Must not be NULL.
 *            On return, this pointer will refer to a newly allocated
 *            buffer containing the hostname. This buffer must be free()ed
 *            by the caller!
 *  
 */
srRetVal sbSock_gethostname(char **psz)
{
	assert(psz != NULL);

    if((*psz = (char*) malloc(256 * sizeof(char))) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	if(gethostname(*psz, 256) != 0)
		return SR_RET_ERR;

	return SR_RET_OK;
}


/**
 * Wrapper for getsockname().
 *
 * Please note: the socket MUST be connected before you can use
 * this method! It should NOT be used on UDP sockets.
 *
 * \param pName Pointer to a struct sockaddr_in that will
 *              receive the remote address. Must not be NULL.
 *
 * \param iNameLen Length of the pName buffer. Updated on return.
 *                 Must not be NULL.
 */
static srRetVal sbSock_getsockname(sbSockObj* pThis, struct sockaddr_in *pName, int* piNameLen)
{
	sbSockCHECKVALIDOBJECT(pThis);
	assert(pName != NULL);
	assert(piNameLen != NULL);
	assert(pThis->sock != INVALID_SOCKET);

	if(getsockname(pThis->sock, (struct sockaddr*) pName, piNameLen) != 0)
		return sbSockSetLastSockError(pThis);

	return SR_RET_OK;
}
