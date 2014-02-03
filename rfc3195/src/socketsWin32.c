/**\file socketsWin32.c
 * \brief Socket layer/driver for Win32
 *
 * This is the socket layer for Win32.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 * \date    2003-08-05
 *			Modified to be a plain lower layer. Moved
 *          some of the code to \ref sockets.c.
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


/* ################################################################# *
 * private members                                                   *
 * ################################################################# */

/** Set last error status. Call as soon as a socket error occured.
 */
static int sbSockSetLastSockError(struct sbSockObject *pThis)
{
	srRetVal iRet;
	pThis->dwLastError = WSAGetLastError();

	switch(pThis->dwLastError)
	{
	case WSAEINVAL:
		iRet = SR_RET_INVALID_SOCKET;
		break;
	default:
		iRet = SR_RET_SOCKET_ERR;
		break;
	}

	return iRet;

}


/* ################################################################# *
 * public members                                                    *
 * ################################################################# */

srRetVal sbSockLayerInit(int bInitOSStack)
{
	WORD wVersionRequested; 
	WSADATA wsaData; 
	int ret; /* for winsock */

	if(bInitOSStack)
	{
		/* because of select(), we need version 2 of sockets */
		wVersionRequested = MAKEWORD(2, 0); 
	 
		ret = WSAStartup(wVersionRequested, &wsaData); 
	 
		if (ret != 0) 
		{
			/* nope, too old :( */ 
			return SR_RET_INVALID_OS_SOCKETS_VERSION;

		}
		else
		{	/* Confirm that the Windows Sockets DLL supports 2.0.*/ 
			/* Note that if the DLL supports versions greater */ 
			/* than 2.0 in addition to 2.0, it will still return */ 
			/* 2.0 in wVersion since that is the version we */ 
			/* requested. */ 
		 	if ( LOBYTE( wsaData.wVersion ) != 2 || 
				 HIBYTE( wsaData.wVersion ) != 0 )
			{ 	/* again, too old :( */ 
				/* ... but we need to do some cleanup in this case! */ 
				WSACleanup(); 
				return SR_RET_INVALID_OS_SOCKETS_VERSION;
			} 
		}
	}

	return SR_RET_OK;
}

srRetVal sbSockLayerExit(int bExitOSStack)
{
	int iRetVal = SR_RET_OK;

	if(bExitOSStack)
	{
		if(WSACleanup() == SOCKET_ERROR)
		{
			iRetVal = SR_RET_ERR;
		}
	}

	return(iRetVal);
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



/* ############################################################ *
 * # Now come more or less winsock wrappers                   # *
 * ############################################################ */


/**
 * Set the socket to nonblocking state.
 */
srRetVal sbSockSetNonblocking(sbSockObj*pThis)
{
	u_long uArg;

	sbSockCHECKVALIDOBJECT(pThis);

	uArg = 1;
	if(ioctlsocket(pThis->sock, FIONBIO, &uArg) != 0)
		return sbSockSetSockErrState(pThis);

	return SR_RET_OK;
}


/**
 * Wrapper for the socket accept() call.
 */
srRetVal sbSockAccept(sbSockObj*pThis, sbSockObj* pNew, struct sockaddr *sa, int *iSizeSA)
{
	sbSockCHECKVALIDOBJECT(pThis);
	sbSockCHECKVALIDOBJECT(pNew);

	if((pNew->sock = accept(pThis->sock, sa, iSizeSA)) == INVALID_SOCKET)
		return sbSockSetSockErrState(pThis);

	return SR_RET_OK;
}


/**
 * Wrapper for the socket listen() call.
 */
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


srRetVal sbSockClosesocket(sbSockObj *pThis)
{
	sbSockCHECKVALIDOBJECT(pThis);

	if(closesocket(pThis->sock) == SOCKET_ERROR)
	{
		sbSockSetSockErrState(pThis);
		return SR_RET_ERR;
	}

	return SR_RET_OK;
}


/**
 * Wrapper for the socket select call on a fd_set structure. 
 * \param fdsetRD fdset structure for read-awaiting sockets.
 *                May be NULL.
 * \param fdsetWR fdset structure for write-awaiting sockets.
 *                May be NULL.
 * \param iTimOutSecs Seconds until timeout. -1 means indefinite
 *        blocking.
 * \param iTimOutMSecs Milliseconds until timeout.
 * \retval Value returned by select.
 */
int sbSockSelectMulti(srSock_fd_set *fdsetRD, srSock_fd_set *fdsetWR, int iTimOutSecs, int iTimOutMSecs)
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

	iRet = select(0, fdsetRD, fdsetWR, NULL, ptv);

	return(iRet);
}


/**
 * Wrapper for the socket select call on a single socket. 
 * \param iTimOutSecs Seconds until timeout. -1 means indefinite
 *        blocking.
 * \param iTimOutMSecs Milliseconds until timeout.
 * \retval Value returned by select.
 */
int sbSockSelect(struct sbSockObject* pThis, int iTimOutSecs, int iTimOutMSecs)
{
	int iRet;
	fd_set fdset;
	struct timeval tv = {iTimOutSecs, iTimOutMSecs};
	struct timeval *ptv;

	sbSockCHECKVALIDOBJECT(pThis);
	FD_ZERO(&fdset);
	FD_SET(pThis->sock, &fdset);
	ptv = iTimOutSecs == -1 ? NULL : &tv;
	iRet = select(0, &fdset, NULL, NULL, ptv);

	return(iRet);
}


int sbSockReceive(struct sbSockObject* pThis, char * pszBuf, int iLen)
{
	int	iBytesRcvd;
	sbSockCHECKVALIDOBJECT(pThis);

	/* iLen - 1 to leave room for the terminating \0 character */
	iBytesRcvd = recv(pThis->sock, pszBuf, iLen - 1, 0);

	if(iBytesRcvd == SOCKET_ERROR)
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
		if(iRetCode == SOCKET_ERROR)
		{
			sbSockSetSockErrState(pThis);
			iRetCode = -1;
		}
	}

	return(iRetCode);
}

srRetVal sbSockConnectoToHost(sbSockObj* pThis, char* pszHost, int iPort)
{
	SOCKADDR_IN remoteaddr;
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

	if(bind(pThis->sock, (LPSOCKADDR) &srv_addr, sizeof(srv_addr)) == SOCKET_ERROR)
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

	iRetCode = connect(pThis->sock, (LPSOCKADDR) &remoteaddr, sizeof(remoteaddr));

	if(iRetCode == SOCKET_ERROR)
	{
		sbSockSetSockErrState(pThis);
		return SR_RET_ERR;
	}

	return SR_RET_OK;
}


/**
 * Bind a socket to the provided address.
 * \param pszAddr Address to bind to. This is a string. If NULL,
 *                no specific address is used.
 *                Please keep in mind that a specific bind to an
 *                address may steal the port and thus NULL could be
 *                causing a security vulnerability.
 * \param iPort	Port to bind to. 
 */
srRetVal sbSockBind(sbSockObj* pThis, char* pszHost, int iPort)
{
	struct sockaddr_in srv_addr;

	sbSockCHECKVALIDOBJECT(pThis);
	assert(pszHost == NULL);	/** sorry, other modes currently not supported \todo implement! */

	/* Bind socket to any port on local machine */
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = INADDR_ANY;

	srv_addr.sin_port = htons(iPort);

	if(bind(pThis->sock, (LPSOCKADDR) &srv_addr, sizeof(srv_addr)) == SOCKET_ERROR)
	{
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
 * \note The Windows sockets API seems to be broken. We often
 *       have seen that only the computer name part (not the domain
 *       name) is returned by gethostname, even if the full
 *       identity is configured. A way to work around this may be
 *       to call gethostbyname() on the returned name and then
 *       call gethostbyaddr() on what is returned there.
 *	     WE HAVE NOW IMPLEMENTED THIS WORKAROUND, but leave
 *       the text above in just in case...
 *
 * \param psz Pointer to Pointer to hostname. Must not be NULL.
 *            On return, this pointer will refer to a newly allocated
 *            buffer containing the hostname. This buffer must be free()ed
 *            by the caller!
 *  
 */
srRetVal sbSock_gethostname(char **psz)
{
	struct hostent *phe;

	assert(psz != NULL);

    if((*psz = (char*) malloc(256 * sizeof(char))) == NULL)
		return SR_RET_OUT_OF_MEMORY;

	if(gethostname(*psz, 256) != 0)
		return SR_RET_ERR;

	/* Now comes the ugly part, but at least it works this
	 * way in Win32... gethostname() does not (always?) return
	 * the FQDN. (@)#!! 
	 */
	phe = gethostbyname(*psz);
	strcpy(*psz, phe->h_name);

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
	assert(fromlen > 0);

	return recvfrom(pThis->sock, buf, len, flags, from, fromlen);
}
