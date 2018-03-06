/**\file socketsUnix.c
 * \brief Socket layer/driver for Unix.
 *
 * This is the low-level socket layer for Unix. This
 * file is included from \ref sockets.c. Please note
 * that this file implements Unix Domain Sockets if that
 * feature is enabled. This is not done in sockets.c, as this
 * obviously is an Unix-only thing.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \author  Devin Kowatch <devink@sdsc.edu>
 *
 * \date    2003-08-04
 *          rgerhards: inital version created
 *
 * \date    2003-08-04
 *          devink: initial UNIX port 
 *
 * \date	2003-08-05
 *          rgerhards: Changed to be a lower layer to the generic sockets.c.
 * 
 * \date	2003-08-05
 *          devink: applied changes to compile under Solaris. AIX also works well.
 *
 * \date	2003-09-29
 *          rgerhards: upgraded this module to support Unix Domain Sockets.
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
#ifdef _AIX
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#if FEATURE_UNIX_DOMAIN_SOCKETS
#	include <sys/un.h>
#endif
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>


/* ################################################################# */
/* private members                                                   */
/* ################################################################# */


/* ################################################################# */
/* public members                                                    */
/* ################################################################# */

srRetVal sbSockLayerInit(int __attribute__((unused)) bInitOSStack)
{
	return SR_RET_OK;
}

srRetVal sbSockLayerExit(int __attribute__((unused)) bExitOSStack)
{
	return SR_RET_OK;
}


sbSockObj* sbSockInit(void)
{
	return(sbSockInitEx(AF_INET, SOCK_STREAM));
}


sbSockObj* sbSockInitEx(int iAF, int iSockType)
{
	struct sbSockObject *pThis;

	assert((iSockType == SOCK_STREAM) || (iSockType == SOCK_DGRAM));

	pThis = (struct sbSockObject*) calloc(1, sizeof(struct sbSockObject));
	if(pThis != NULL)
	{	/* initialize class members */
		if((pThis->sock = socket(iAF, iSockType, 0)) == INVALID_SOCKET)
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
    memset(&remoteaddr, 0, sizeof(remoteaddr));
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

	if((pNew->sock = accept(pThis->sock, sa, (socklen_t*)iSizeSA)) == INVALID_SOCKET)
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
    memset(&srv_addr, 0, sizeof(srv_addr));
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
	socklen_t len = *piNameLen;

	int result = getsockname(pThis->sock, (struct sockaddr*) pName, &len);

	*piNameLen = (int)len;
	
	return (result != 0) ? sbSockSetLastSockError(pThis) : SR_RET_OK;
}

/**
 * Wrapper for recvfrom().
 */
static int sbSock_recvfrom(sbSockObj *pThis, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
{
	sbSockCHECKVALIDOBJECT(pThis);
	assert(pThis->sock != INVALID_SOCKET);
	assert(buf != NULL);
	assert(len > 0);
	assert(from != NULL);
	assert(*fromlen > 0);
	socklen_t _fromlen = *fromlen;
	
	int result = recvfrom(pThis->sock, buf, len, flags, from, &_fromlen);

	*fromlen = (int)_fromlen;

	return result;
}

#if FEATURE_UNIX_DOMAIN_SOCKETS == 1
/**
 *
 */
srRetVal sbSock_InitUXDOMSOCK(sbSockObj **ppThis, char *pszSockName, int iSockType)
{
	srRetVal iRet;
	struct sockaddr_un sa;

	assert(ppThis != NULL);
	assert(pszSockName != NULL);

	if(*pszSockName == '\0')
		return SR_RET_INVALID_PARAM;

	if((*ppThis = sbSockInitEx(AF_UNIX, SOCK_DGRAM)) == NULL)
		return SR_RET_CAN_NOT_INIT_SOCKET;

	/* socket allocated, now use it */
	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, pszSockName, sizeof(sa.sun_path) - 1);
	sa.sun_path[sizeof(sa.sun_path) - 1] = '\0'; /* ensure no overrun */

	unlink(sa.sun_path); /* don't use previous instance (if there) */

	if(bind((*ppThis)->sock, (struct sockaddr *) &sa, sizeof(sa.sun_family)+strlen(sa.sun_path)) < 0)
		return SR_RET_CANT_BIND_SOCKET;

	/* make it world-writable */
	if(chmod(sa.sun_path, 0666) < 0)
		return SR_RET_UXDOMSOCK_CHMOD_ERR;

	return SR_RET_OK;
}

#endif /* FEATURE_UNIX_DOMAIN_SOCKETS */
