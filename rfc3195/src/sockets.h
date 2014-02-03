/*! \file sockets.h
 *  \brief socket layer for slim beep
 * 
 * This implements a very thin layer above the socket
 * subsystem. If you intend to port the lib to a
 * different environment, you should probably stick with
 * that layer and create a new socketsXXX.c file.
 *
 * The prefix for this file is sbSock.
 *
 * All methods receive a pointer to their
 * "object instance" as the first parameter.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 *          0.1 version created.
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
#ifndef __LIB3195_SOCKETS_H_INCLUDED__
#define __LIB3195_SOCKETS_H_INCLUDED__ 1

#include "settings.h"
#ifdef SROS_WIN32
#  include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#endif

#define sbSockCHECKVALIDOBJECT(x) {assert(x != NULL); assert(x->OID == OIDsbSock);}

/** The value to return when no character could be read
 *  by a socket recv call.
 */
#define sbSOCK_NO_CHAR -1

struct sbSockObject 
/** The socket object. This works much like the implied
 * object created by C++.
 */
{	
	srObjID OID;		/**< object ID */
	srRetVal iLastErr;  /**< last error code from liblogging - mostly used in cases where a pointer is returned */
	SR_SOCKET sock;		/**< the actual socket */
	int bIsInError;		/**< was there an error in this layer? */
	int	dwLastError;	/**< last error code reported by stack */
	char szInBuf[SOCKETMAXINBUFSIZE]; /**< input buffer \see SOCKETMAXINBUFSIZE for details. */
	int iCurInBufPos;	/**< current position in input buffer. This is the LAST character that was processed */
	int iInBufLen;		/**< current length of inBuf. 0 means empty */
	fd_set fdset;		/**< this may not be portable on some exotic OS, but so far I have no better idea to hide it... */
#	if FEATURE_LISTENER == 1
	struct sockaddr_in RemoteHostAddr;/**< ipAddr of the remote host [sorry, IP V4 only currently... :-(] */
	char *pRemoteHostIP;	/**< IP address of the remote host connected to this socket */
	int iRemHostIPBufLen;	/**< strlen()+ 1 of the remote host IP address (for malloc()) */
#	endif
};
typedef struct sbSockObject sbSockObj;


/* The following three macros could also be inline code.
 * For now, I have kept them to be macros to make 
 * facilitate porting.
 */

/** Getthe last error for this module */
#define sbSockGetLastError(pThis) pThis->iLastErr

/** Set the last error for this module */
#define sbSockSetLastError(pThis, iRet) pThis->iLastErr = iRet

/** reset the last error for this module */
#define sbSockResetLastError(pThis) pThis->iLastErr = SR_RET_OK


/** Global Layer initialization. Should be called only once per
 *  application. This will do all housekeeping necessary to
 *  use the layer. DO NOT CALL ANY OTHER METHODS BEFORE THIS
 *  ONE HAS BEEN CALLED! They will probably fail!
 *
 * \param bInitOSStack Specifies if the library should initialize
 *        the operating system socket stack. If 1, it
 *        initializes it, if 0, it does not. Set this to 0
 *        if you integrate this lib into an app that otherwise
 *        initializes the socket lib itself.
 */
srRetVal sbSockLayerInit(int bInitOSStack);	
/** Global Layer destructor. Call this only once at the
 *  end of the application. All sockets must be destroyed
 *  before calling this method.
 *
 * \param bExitOSStack Specifies if the library should shut down
 *        the operating system socket stack. If 1, it
 *        shuts it down, if 0, it does not. Set this to 0
 *        if you integrate this lib into an app that otherwise
 *        shuts down the OS socket lib itself.
 */
srRetVal sbSockLayerExit(int bExitOSStack);

sbSockObj* sbSockInit(void);	/**< Constructor for STREAM sockets, only \retval Returns a pointer to the new instance or NULL, if it could not be created.  */
sbSockObj* sbSockInitEx(int af, int iSockType); /**< Alternate Constructur. Socket TYPE (STREAM/DGRAM) can be specified */

srRetVal sbSockExit(sbSockObj*);	/**< Destructor */
int sbSockGetLastSockError(sbSockObj*);	/**< get last error code. \retval last error code (OS specific) */

/** Connect a socket to a remote host.
 *  \param pszHost Name or IP address of host to connect to
 *  \param iPort Port to connect to
 */
srRetVal sbSockConnectoToHost(sbSockObj* pThis, char* pszHost, int iPort);

/** Check if the socket has data ready to be received.
 *  \retval TRUE if data is ready, FALSE otherwise. The
 *          return value is only valid if no error occured. To
 *          make your app bullet-proof, you need to check the
 *          error indicator before using the retval... ;)
 */
int sbSockHasReceiveData(sbSockObj* pThis);

/** Receive data from the socket. This is done in a blocking way.
 *  If you would not like to block, call \ref sbSockHasReceiveData
 *  before calling this method.
 *
 *  \param *pszBuf [in] - data to be send
 *  \param iLen [in] - length of pszBuf
 *  \retval actual bytes sent by socket subsystem
 */
int sbSockReceive(sbSockObj* pThis, char * pszBuf, int iLen);

/** Send the supplied buffer.
 *  \param *pszBuf [in] - data to be sent.
 *         If NULL, no data is sent (but NULL is a VALID 
 *         value for this param!).
 *  \param iLen [in] - number of bytes to send
 *  \retval number of bytes actually sent
 */
int sbSockSend(sbSockObj* pThis, const char* pszBuf, int iLen);

/**
 * Read the next character from the stream. The character
 * is removed from the stream. If no data is ready on the
 * stream, a blocking read is carried out. So this function
 * can block!
 *
 * \retval character read
 */
int sbSockGetRcvChar(sbSockObj *pThis);

/**
 * Peek at the next character from the stream. The character
 * is NOT removed from the stream. If no data is ready on the
 * stream, a blocking read is carried out. So this function
 * can block!
 *
 * \retval character read
 */
int sbSockPeekRcvChar(sbSockObj *pThis);

/**
 * Close a socket.
 */
srRetVal sbSockClosesocket(sbSockObj *pThis);


/**
 * Wait until the socket has some receive data. This
 * is a blocking call.
 */
srRetVal sbSockWaitReceiveData(struct sbSockObject* pThis);

/**
 * Wrapper for the fd_set data structure
 */
typedef fd_set srSock_fd_set;

/**
 * wrapper for the FD_ZERO macro.
 * This may not work on every exotic OS, but it
 * gives at least a small hook to porting it...
 * Semantics are as with the original FD_ZERO.
 */
#define sbSockFD_ZERO(fdset) FD_ZERO(fdset)

/** Wrapper for the FD_SET macro.
 * This may not work on every exotic OS, but it
 * gives at least a small hook to porting it...
 * Semantics are as with the original FD_ZERO.
 */
#define sbSockFD_SET(sock, fdset) FD_SET(sock, fdset)

/** Wrapper for the FD_ISSET macro.
 * This may not work on every exotic OS, but it
 * gives at least a small hook to porting it...
 * Semantics are as with the original FD_ZERO.
 */
#define sbSockFD_ISSET(sock, fdset) FD_ISSET(sock, fdset)


/**
 * Wrapper for the socket select call on a fd_set structure. 
 * \param iTimOutSecs Seconds until timeout. -1 means indefinite
 *        blocking.
 * \param iTimOutMSecs Milliseconds until timeout.
 * \param iHighestDesc [*NIX ONLY!] The highest file descriptor in
 *        any of the fd_sets.
 * \retval Value returned by select.
 */
#	ifdef SROS_WIN32
		int sbSockSelectMulti(srSock_fd_set *fdsetRD, srSock_fd_set *fdsetWR, int iTimOutSecs, int iTimOutMSecs);
#	else
		int sbSockSelectMulti(srSock_fd_set *fdsetRD, srSock_fd_set *fdsetWR, int iTimOutSecs, int iTimOutMSecs, int iHighestDesc);
#	endif

/**
 * Wrapper for the socket listen() call.
 */
srRetVal sbSockListen(sbSockObj*pThis);

/**
 * Initialize a listen socket. This includes
 * everything so that the next call can be
 * listen().
 *
 * \param iRet [out] Pointer to a variable holding the return
 *                   code of this operation. Has the error
 *                   code if initialisation failed.
 * \param iType sockets type to be used. Must be either
 *              SOCK_STREAM or SOCK_DGRAM.
 * \param szBindToAdresse string with IP address to which the
 *		  socket should be bound. If NULL, do not bind to any
 *	      specific address.
 * \param iBindToPort	Port the socket should be bound to. For
 *        listening sockets, this is the port that the remote
 *        peer will to connect to.
 * \retval Pointer to newly created socket or NULL, if an
 *         error occured. In case of NULL, the caller can
 *         examine the socket error code to learn the error
 *         cause.
 */
sbSockObj* 	sbSockInitListenSock(srRetVal *iRet, int iType, char *szBindToAddress, unsigned uBindToPort);

/**
 * This method accepts an incoming connection and creates
 * a new socket object out of it.
 */
srRetVal sbSockAcceptConnection(sbSockObj* pThis, sbSockObj** pNew);

/**
 * Set the socket to nonblocking state.
 */
srRetVal sbSockSetNonblocking(sbSockObj*pThis);

/**
 * Return the IP Address of the remote host as a string.
 * The caller must free the returned string.
 */
srRetVal sbSockGetRemoteHostIP(sbSockObj *pThis, char **ppszHost);

/**
 * Wrapper for gethostname().
 *
 * \param psz Pointer to Pointer to hostname. Must not be NULL.
 *            On return, this pointer will refer to a newly allocated
 *            buffer containing the hostname. This buffer must be free()ed
 *            by the caller!
 *  
 */
srRetVal sbSock_gethostname(char **psz);

/**
 * Provide the (local) IP address this session is sending to the
 * remote peer on as a string.
 *
 * OK, the short description is probably not very well ;) What it
 * does is, it looks up the local IP address that is used to
 * talk to the remote Peer. This is especially important on 
 * multihomed machines, as different interfaces can be used
 * to forward data (depending on which one is closest to the
 * recipient). Then, it formats this as an IP Address string that
 * then is handed back to the caller. 
 *
 * Please note: the socket MUST be connected before you can use
 * this method! It should NOT be used on UDP sockets.
 *
 * \param ppsz Pointer to a char* that will receive the IP string.
 *             The caller must free the returned pointer when he
 *             is done. The provided pointer must not be NULL.
 */
srRetVal sbSockGetIPusedForSending(sbSockObj* pThis, char**ppsz);


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
srRetVal sbSockRecvFrom(sbSockObj *pThis, char* pRecvBuf, int *piBufLen, char** ppFrom);


#ifdef SROS_WIN32
#	define SBSOCK_EWOULDBLOCK WSAEWOULDBLOCK
#else
#	define SBSOCK_EWOULDBLOCK EWOULDBLOCK
#	define SOCKET_ERROR -1
#endif

#endif
