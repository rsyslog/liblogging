/*! \file beeplisten.h
 *  \brief The BEEP Listener Object
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-13
 *          first version begun.
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
};
typedef struct sbLstnObject sbLstnObj;


/**
 * Initialize a BEEP listener. The listener socket is
 * created, but it is not yet set to listen() to incoming
 * calls.
 *
 * \param pThis {out] Pointer to a pointer to the object. On return,
 *	            it contains a pointer to a new BEEP listener object or NULL,
 *				if an error occured. Consult the return value to learn more
 *              in the later case.
 */
srRetVal sbLstnInit(sbLstnObj** pThis);

/**
 * THE BEEP listener.
 * See \ref listeners Listeners for details.
 */
srRetVal sbLstnRun(sbLstnObj* pThis);

/**
 * Exit a BEEP listener. Will do all cleanup necessary.
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

#endif
