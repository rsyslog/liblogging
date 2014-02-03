/*! \file beeplisten.h
 *  \brief The Listener Object.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-13
 *          first version begun.
 * \date    2003-09-29
 *          Changed this object so that it is no longer "just" BEEP
 *          listener but a general listener. As of 2003-09-29, it 
 *          supports UDP (RFC 3164) in addition to BEEP. Later versions
 *          will probably support other transports, too.
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
#ifndef __LIB3195_BEEPLISTEN_H_INCLUDED__
#define __LIB3195_BEEPLISTEN_H_INCLUDED__ 1
#define sbLstnCHECKVALIDOBJECT(x) {assert(x != NULL); assert(x->OID == OIDsbLstn);}

/** The beep listener object. Implemented via \ref beeplisten.h and
 *  \ref beeplisten.c.
 */
struct sbLstnObject
{	
	srObjID OID;					/**< object ID */
	struct sbSockObject* pSockListening;	/**< our own listening socket */
	struct sbNVTRObject* pRootSessions;		/**< this server's active sessions */
	struct sbNVTRObject* pProfsSupported;	/**< list of supported profiles */
	char* szListenAddr; /**< IP address the server should bind to - NULL means no specific address */
	unsigned uListenPort; /**< port the server should bind to */
	int	bRun;			  /**< run indicator for server. If set to FALSE, server will terminate */
	int bLstnBEEP;		  /**< should we listen to BEEP (RFC3195)? */
	struct srAPIObject *pAPI;	/**< pointer to our API Object */
#	if FEATURE_UDP == 1
	/* now come the selectors for different listeners. Remember, we are no
	 * longer BEEP only (2003-09-29 RGerhards)
	 */
	int bLstnUDP;		/**< should we listen on plain UDP (RFC 3164)? TRUE/FALSE */
	unsigned uUDPLstnPort;	/**< if listening on UDP, this is the port. 0 means default (services lookup) */
	struct sbSockObject* pSockUDPListening;	/**< our listening socket for UDP */
#	endif /* FEATURE_UDP */
#	if FEATURE_UNIX_DOMAIN_SOCKETS == 1
	/* now come the selectors for different listeners. Remember, we are no
	 * longer BEEP only (2003-09-29 RGerhards)
	 */
	int bLstnUXDOMSOCK;	/**< should we listen on Unix Domain Sockets? TRUE/FALSE */
	char *pSockName;	/**< if listening on domain sockets, this is the name of the socket */
	struct sbSockObject* pSockUXDOMSOCKListening;	/**< our listening socket for UDP */
#	endif /* FEATURE_UNIX_DOMAIN_SOCKETS */
};
typedef struct sbLstnObject sbLstnObj;


/**
 * Initialize a listener. The listener socket is
 * created, but it is not yet set to listen() to incoming
 * calls.
 *
 * \param pThis {out] Pointer to a pointer to the object. On return,
 *	            it contains a pointer to a new BEEP listener object or NULL,
 *				if an error occured. Consult the return value to learn more
 *              in the later case.
 */
srRetVal sbLstnInit(sbLstnObj* pThis);

/**
 * THE listener process. As of 2003-09-29, this no longer is
 * a pure BEEP listener but THE listener who listens to all
 * configured transports, e.g. UDP and BEEP.
 * See \ref listeners Listeners for details.
 */
srRetVal sbLstnRun(sbLstnObj* pThis);

/**
 * Exit the listener. Will do all cleanup necessary.
 */
srRetVal sbLstnExit(sbLstnObj *pThis);


/**
 * Add a profile object to the current listener object.
 * The profile becomes active with the next greeting.
 * For convenience, the key is set to the profile URI.
 */
srRetVal sbLstnAddProfile(struct sbLstnObject *pThis, struct sbProfObject *pProf);

/**
 * Exit a listener object.
 */
srRetVal sbLstnExit(sbLstnObj *pThis);

/**
 * Destructor for the beep listener object.
 */
void sbLstnDestroy(sbLstnObj* pThis);

/**
 * Construct a sbLstnObj.
 *
 * This MUST be the first call a server makes. After the
 * server has obtained the sbLstnObj, it can set the parameters
 * and THEN call sbLstnInit().
 *
 * \retval pointer to constructed object or NULL, if
 *         error occurred.
 */
sbLstnObj* sbLstnConstruct(void);

#endif
