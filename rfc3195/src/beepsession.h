/*! \file beepsession.h
 *  \brief The BEEP Session Object
 *
 * \note The current implementation only supports 2 channels -
 *       the control channel plus 1 payload channel.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 *          0.1 version created.
 *
 * \date    2003-08-07
 *          name value list for channnels added.
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
#ifndef __LIB3195_BEEPSESSION_H_INCLUDED__
#define __LIB3195_BEEPSESSION_H_INCLUDED__ 1
#define sbSessCHECKVALIDOBJECT(x) {assert((x) != NULL); assert((x)->OID == OIDsbSess);}

struct sbMesgObject;

#include "beepchannel.h"
#include "beepprofile.h"

#if FEATURE_LISTENER == 1
	/** Session status */
	enum sbSessState_
	{
		sbSESSSTATE_OPEN,
		sbSESSSTATE_CLOSED,			/**< session is closed and ready to be removed from
									 **  whatever queues it is in.
									 **/
		sbSESSSTATE_UNKNOWN = 0		/**< should never happen. Set after initialization. */
	};
	typedef enum sbSessState_ sbSessState;

	enum sbSessTXState_
	{
		sbSESSTXSTATE_NO_TX_PENDING,	/**< no transmit pending */
		sbSESSTXSTATE_IN_TX,			/**< data is currently being send */
		sbSESSTXSTATE_UNKNOWN = 0		/**< should never happen. Set after initialization. */
	};
	typedef enum sbSessTXState_ sbSessTXState;
#endif

struct sbChanObject;
struct sbChanObject;
struct sbFramObject;
struct sbNVTRObject;
struct sbSessObject 
/** The session object. Implemented via \ref beepsession.h and
 *  \ref beepsession.c.
 */
{	
	srObjID OID;						/**< object ID */
	srRetVal iLastErr;					/**< last error occured */
	struct sbSockObject *pSock;			/**< underlying socket */
	struct sbChanObject *pChan0;		/**< the control channel. This is stored in a separate variable for
										 **  performance reasons. It also has an entry in pChannels. */
	struct sbNVTRObject *pChannels;		/**< pointer to a name value list with the channels. */
	struct sbNVTRObject* pRXQue;		/**< receive frame queue */
	struct sbNVTRObject *pRemoteProfiles; /**< linked list of profiles supported by remote peer. Constructed based on greeting. */
	srRetVal (*SendFramMethod)(struct sbSessObject *, struct sbFramObject *, struct sbChanObject *); /**< method to be
											used for sending the frame. Is different for client and server implementations */
	struct sbNVTRObject *pProfilesSupported;/**< the list of profiles supported at the time of session connection. */
#if FEATURE_LISTENER == 1
	sbSessState iState;						/**< this session's current status */
	sbSessTXState iTXState;					/**< session transmit status */
	struct sbNVTRObject *pSendQue;			/**< queue of data to be send */
	struct sbFramObject *pRecvFrame;		/**< frame currently being received */
	int	bNeedData;							/**< TRUE = can NOT send, because data (SEQ frame) needs to arrive first */
#endif
};
typedef struct sbSessObject sbSessObj;

/** Open a BEEP session.
 *  The initial greeting is sent and the remote peers initial greeting
 *  is accepted. Channel 0 is initialized and brought into operation.
 *  
 * \param pszRemotePeer name or IP address of the peer to connect to
 * \param iPort TCP port the remote peer is listening to
 * \retval pointer to the new session object or NULL, if an error
 *         occured.
 */
sbSessObj* sbSessOpenSession(char* pszRemotePeer, int iPort, struct sbNVTRObject *pProfSupported);
srRetVal sbSessCloseSession(sbSessObj *pThis);


/* The following three macros could also be inline code.
 * For now, I have kept them to be macros to make 
 * facilitate porting.
 */

/** Getthe last error for this module */
#define sbSessGetLastError(pThis) pThis->iLastErr

/** Set the last error for this module */
#define sbSessSetLastError(pThis, iRet) pThis->iLastErr = iRet

/** reset the last error for this module */
#define sbSessResetLastError(pThis) pThis->iLastErr = SR_RET_OK

/**
 * Retrieve the channel object associated with the channel number.
 * This method allows to retrieve a channel number based on the 
 * actual channel.
 * 
 * \param uChanNo channel number
 * \retval pointer to channel object or NULL, if uChanNo could
 *         not be found.
 */
struct sbChanObject* sbSessRetrChanObj(sbSessObj *pThis, SBchannel uChanNo);

/**
 * Receive a frame for a specified channel from the tcp stream.
 * This method handles intermixed frames from other channels. It
 * calls back into the frame object to do the actual frame read
 * (if no frame is buffered).
 * 
 * See \ref architecture.c for a full description.
 *
 * \param pChan the associated channel
 * \retval pointer to the new frame object or NULL, if an error
 *         occured.
 */
struct sbFramObject* sbSessRecvFram(sbSessObj* pThis, struct sbChanObject  *pChan);

/**
 * Open a BEEP channel on an established session.
 * The method carries out all channel 0 management functions
 * to open up a new channel.
 *
 * When the method returns without error, the channel is ready 
 * for data transmission. If the remote peer send a (profile payload) 
 * frame as part of the channel creation, this frame is probably 
 * available for retrieval.
 *
 * \param pProfsSupported [in] pointer to the profile supporeted for this session.
 */
struct sbChanObject* sbSessOpenChan(sbSessObj* pThis);

/**
 * Close a channel.
 * This is a very partial implementation of RFC 3080. For syslog-raw,
 * I honestly and strongly beliefe it works. However, there may be
 * implementations of syslog raw where it will not work or at least
 * not work fully correctly.
 *
 * \todo This method (and some of the plumbing behind) must be
 *       updated to be fully compliant to RFC 3080.
 * 
 * Issues are:
 * - we do not expect other messages to come in during the
 *   session
 * - if the remote peer closes the session before us, we
 *   may be in trouble
 *
 * \param pChan [in] Channel to be closed. Will be destroyed
 *                   at the end of the execution and thus can
 *                   no longer be used after this method call!
 */
srRetVal sbSessCloseChan(sbSessObj *pThis, struct sbChanObject *pChan);

/**
 * Closes the BEEP session. First, the management channel (channel 0)
 * is taken down and then the TCP stream is closed.
 * This function can only be called after all sessions have been closed.
 * Once the function returns, the session object is no longer valid
 * and can no longer be used.
 */
srRetVal sbSessCloseSession(sbSessObj *pThis);

/**
 * Receive frames off the tcp stream. SEQ frames
 * are processed as they arrive. Other frames are
 * buffered in the respective in-memory queue. This is
 * a helper to sbSessRecvFram() and sbSessSendFram().
 *
 * \param bMustRcvPayloadFrame A boolean indicating if an
 *       actual payload frame must be received. If set to
 *       TRUE, the method blocks until a payload frame is
 *       received. All interim SEQ frames are processed, but
 *       will not make this method to return.
 *       If FALSE, the method will block until anything is received,
 *       including a SEQ. If a SEQ is received, it will be processed
 *       and the method returns.
 */
srRetVal sbSessDoReceive(sbSessObj *pThis, int bMustRcvPayloadFrame);

/**
 * Send the provided frame on the provided channel. While sending,
 * concurrently incoming frames are buffered and SEQ frames are 
 * processed. This method blocks until pFram could be sent (this
 * means it may need to wait for the proper SEQ frame).
 *
 * See \ref architecture.c for full details on how this
 * method "drives the BEEP stack".
 *
 * \param pFram [in] frame to be send
 * \param pChan [in] channel where pFram is to be sent over
 */
srRetVal sbSessSendFram(sbSessObj *pThis, struct sbFramObject *pFram, struct sbChanObject *pChan);

/** Destructor. pThis is invalid
 *  when this function returns. The caller can no longer use
 *  it. The caller is also not allowed to free it - this is
 *  already done in this method.
 */
void sbSessDestroy(sbSessObj *pThis);

/**
 * Open a BEEP session. This method is called
 * if a REMOTE peer requests an session open. As such,
 * it does only the most pressing initialization work and
 * then returns back to the caller.
 */
srRetVal sbSessRemoteOpen(sbSessObj **pThis, sbSockObj *pSock, struct sbNVTRObject *pProfSupported);

/**
 * Close a session without any further communication. Can
 * be used to abort a connection if
 * - the remote peer broke it
 * - we have a protocol error and we need to break it
 *
 * This method is void, because it *must* complete, no matter
 * which errors it encounters. Furthermore, it is primarily
 * intended to be called from destructors which are also void.
 * Consider it to be an extended desctrutor.
 */
void sbSessAbort(sbSessObj *pThis);

/**
 * Process the remote peer's greeting message. Most importantly,
 * the list of profiles is extracted and stored in the session
 * object.
 *
 * \param pMesg The message containing the Greeting. Must not be NULL.
 */
srRetVal sbSessProcessGreeting(struct sbSessObject *pThis, struct sbMesgObject *pMesg);

/**
 * Send a greeting message based on the currently registered
 * profiles.
 *
 * We generate the greeting via a simple stringbuf, not with
 * an XML parser. This saves execution time and is as good
 * as the XML thing...
 *
 * \param pProfsSupported Pointer to a linked list of supported
 *        profiles (which should be specified in the greeting.
 */
srRetVal sbSessSendGreeting(struct sbSessObject* pSess, struct sbNVTRObject *pProfsSupported);


#endif
