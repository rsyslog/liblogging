/*! \file srAPI.h
 *  \brief The syslog reliable API.
 *
 * This is the transport part of the API. Please be sure
 * to also look at \ref syslogmessage.h for the API
 * on the syslog message itself.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 *          0.1 version created.
 * \date    2003-10-02
 *          Added a number of library options so that the different
 *          listeners can be flexibly configured.
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
#ifndef __LIB3195_SRAPI_H_INCLUDED__
#define __LIB3195_SRAPI_H_INCLUDED__ 1

#ifdef __cplusplus
extern "C" {
#endif

#define srAPICHECKVALIDOBJECT(x) {assert(x != NULL); assert(x->OID == OIDsrAPI);}
struct srSLMGObject;

struct srAPIObject
/** The syslog-reliable API object. Implemented via \ref srAPI.h and
 *  \ref srAPI.c.
 */
{	
	srObjID OID;					/**< object ID */
	struct sbChanObject *pChan;		/**< The BEEP channel associated with this API instance */
	struct sbNVTRObject *pProfsSupported;/**< the supported profiles by this API (initiator only) */
	struct sbSessObject *pSess;		/**< the associated session */
	void *pUsr;						/**< for the user's use - not handled in any way in this lib */
	/** \todo create the methods for pUsr */
	srOPTION3195Profiles iUse3195Profiles; /**< specifies which profiles to use for 3195 */
#	if FEATURE_LISTENER == 1
	int bListenBEEP;			/**< TRUE/FALSE - listen to udp? */
	int iBEEPListenPort;		/**< port to use when listening on BEEP */
	void (*OnSyslogMessageRcvd)(struct srAPIObject* pAPI, struct srSLMGObject *pSyslogMesg);
	struct sbLstnObject *pLstn;	/**< pointer to associated listener object */
	int bListenUDP;				/**< TRUE/FALSE - listen to udp? */
	int iUDPListenPort;			/**< port to use when listening on UDP */
	int bListenUXDOMSOCK;		/**< TRUE/FALSE - listen to unix domain socket? */
	char *szNameUXDOMSOCK;		/**< pointer to c-string with socket name */
#	endif
};
typedef struct srAPIObject srAPIObj;

/**
 * Initialize the liblogging library. This method must be called
 * before any other method.
 *
 * \retval Pointer to an API object (aka "handle") or NULL
 *         if an error occured.
 */
srAPIObj* srAPIInitLib(void);

/** Open a log session.
 * 
 * \param pszRemotePeer [in] The Peer to connect to.
 * \param iPort [in] The port the remote Peer is listening to.
 * \retval Pointer to an API object (aka "handle") or NULL
 *         if an error occured.
 */
srRetVal srAPIOpenlog(srAPIObj *pThis, char* pszRemotePeer, int iPort);


/**
 * Close a syslog raw session.
 * In the method, we have a slightly different error handling. We do
 * NOT return immediately when something goes wrong but rather preserve
 * the error state and try to continue processing. The reasoning 
 * behind this is that we can potentially create an even larger
 * memory leak if we do not try to continue. After all, we are
 * here to free things and shutdown the process. The only exception
 * is when we detect that the caller-provided handle is invalid.
 * In this case, we CAN NOT continue processing.
 * 
 * When this method returns, the the caller-supplied pointer
 * to the API object is NO LONGER VALID and can not be used
 * for consequtive calls.
 */
srRetVal srAPICloseLog(srAPIObj *pThis);

/**
 * Exit the liblogging library. This method should be called after 
 * a higher layer is COMPLETE DONE with liblogging. It does not
 * need to be called after each session close.
 *
 * if srAPIExitLib() is called, the lib can not be used
 * until srAPIInitLib() is called again.
 *
 * The provided pointer to the API object is invalid
 * once this method returns.
 */
srRetVal srAPIExitLib(srAPIObj *pThis);



/**
 * Send a log message to the remote peer. The log message must
 * already be correctly formatted according to RFC 3195. No
 * further formatting (or checks) are applied.
 *
 * \param pThis [in] pointer to a proper srAPI object.
 * \param szLogmsg [in] message to be sent.
 */
srRetVal srAPISendLogmsg(srAPIObj* pThis, char* szLogmsg);

/**
 * Send a syslog message object to the remote peer. This
 * method SHOULD be called whenever the caller is already
 * in possesion of a srSLMGObj. Calling srAPISendLogmsg would
 * result in a performance penalty in those cases. In general,
 * you should call this method here whenever you can and use
 * srAPISendLogmsg only in those (rare) cases where you
 * can not avoid it.
 *
 * \param pThis [in] pointer to a proper srAPI object.
 * \param pSLMG pointer to the syslog message object. This
 *              must be a proper object as it will be
 *              used as the message source.
 */
srRetVal srAPISendSLMG(srAPIObj* pThis, struct srSLMGObject* pSLMG);

/**
 * Close a syslog raw session.
 * In the method, we have a slightly different error handling. We do
 * NOT return immediately when something goes wrong but rather preserve
 * the error state and try to continue processing. The reasoning 
 * behind this is that we can potentially create an even larger
 * memory leak if we do not try to continue. After all, we are
 * here to free things and shutdown the process. The only exception
 * is when we detect that the caller-provided handle is invalid.
 * In this case, we CAN NOT continue processing.
 * 
 * When this method returns, the the caller-supplied pointer
 * to the API object is NO LONGER VALID and can not be used
 * for consequtive calls.
 */
srRetVal srAPICloseLog(srAPIObj *pThis);


/**
 * Set an library option. Library options provide a 
 * way to modify library behaviour. For example, 
 * once rfc3195 raw and cooked are implemented, you can
 * set an option to set which transport you would like
 * to have. Consequently, it currently looks like
 * the calling sequence is probably always:
 *
 * 1. srAPIInitLib
 * 2. srAPISetOption
 * 3. srAPIOpenlog
 * 4. srAPISendLogmsg
 * 5. srAPICloselog
 * 6. srAPIExitLib
 *
 * \param pThis pointer to the API, as usual. The unusual
 *        things is that in this case it may be NULL. If so,
 *        global settings will be modified on a per-library
 *        basis.
 * \param iOpt option to set
 * \param iOptVal value that the option is to be set to.
 *        The option value is totally dependent on the
 *        respective option.
 */
srRetVal srAPISetOption(srAPIObj* pThis, SRoption iOpt, int iOptVal);
/* And the same method for string options... */
srRetVal srAPISetStringOption(srAPIObj* pThis, SRoption iOpt, char *pszOptVal);

/**
 * Run the listener process. Control is only returned if
 * - a fatal error inside the listener happens
 * - srAPIShutdownListener() has been called
 *
 * As control does not return, please note that srAPIShutdownListener()
 * must be called either from another thread or a signal handler.
 */
srRetVal srAPIRunListener(srAPIObj *pThis);

/**
 * Shut down the currently running listener. It
 * may take some time to shutdown the listener depending
 * on its current state. The caller should expect a delay of
 * up to 30 seconds.
 */
srRetVal srAPIShutdownListener(srAPIObj *pThis);

/**
 * Set the user pointer. Whatever value the user
 * passes in is accepted and stored. No validation
 * is done (and can be done). Any previously stored
 * information is overwritten.
 */
srRetVal srAPISetUsrPointer(srAPIObj *pAPI, void* pUsr);

/**
 * Return the user pointer. Returns whatever is stored
 * in pUsr.
 *
 * \param ppToStore Pointer to a pointer that will receive
 *                  the user pointer.
 */
srRetVal srAPIGetUsrPointer(srAPIObj *pAPI, void **ppToStore);

/**
 * Setup the listener for this API object. The listener is 
 * initialized but not running. Only a single listener can
 * be active for a single API object.
 *
 * \param NewHandler Pointer to a function with C calling conventions
 * the will receive the syslog message object as soon as it arrives.
 * This may be NULL. In this case, however, no meaningful work is done.
 * Syslog messages arrive but will be discarded because there is 
 * no upper-layer peer.
 */
srRetVal srAPISetupListener(srAPIObj* pThis, void(*NewHandler)(srAPIObj*, struct srSLMGObject*));

/**
 * Set a handler to be called when a syslog message
 * arrives. If this method is called when there already a new handler
 * is set, the previous one is discarded. There can only be a single
 * handler set at a single time.
 *
 * \param NewHandler Pointer to a function with C calling conventions
 * the will receive the syslog message object as soon as it arrives.
 * This may be NULL. In this case, however, no meaningful work is done.
 * Syslog messages arrive but will be discarded because there is 
 * no upper-layer peer.
 */
srRetVal srAPISetMsgRcvCallback(srAPIObj* pThis, void(*NewHandler)(srAPIObj*, struct srSLMGObject*));

/**
 * Shut down the currently running listener. It
 * may take some time to shutdown the listener depending
 * on its current state. The caller should expect a delay of
 * up to 30 seconds.
 */
srRetVal srAPIShutdownListener(srAPIObj *pThis);


#ifdef __cplusplus
};
#endif

#endif
