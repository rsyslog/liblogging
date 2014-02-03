/*! \file beepchannel.h
 *  \brief global objects and defines
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 *
 * \date    2003-09-04
 *          Updated to support multiple client profiles.
 *
 * \date    2003-09-08
 *          Added support for sending SEQs in reply to the server's
 *          response.
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
#ifndef __LIB3195_BEEPCHANNEL_H_INCLUDED__
#define __LIB3195_BEEPCHANNEL_H_INCLUDED__ 1
#include "sockets.h"
#include "beepsession.h"

/** Invalid channel number (no valid channel value) */
#define SBCHAN_INVALID_CHANNEL -1
#define sbChanCHECKVALIDOBJECT(x) {assert((x) != NULL); assert((x)->OID == OIDsbChan); assert((x)->iState != sbChan_STATE_INVALID);}

/**
 * The status of the channel. We may not need all of these
 * values - so far defined those that we think to be useful.
 */
enum sbChannelState_				/** return value. All methods return this if not specified otherwise */
{
	sbChan_STATE_INVALID = 0,		/**< should never occur, indicates a program error */
	sbChan_STATE_INITIALIZED,		/**< The channel object is initialized, but the channel not yet opened */
	sbChan_STATE_OPEN,				/**< channel is open and ready for data communication */
	sbChan_STATE_AWAITING_CLOSE,	/**< peer has indicated processing is done, channel is awaiting close command */
	sbChan_STATE_PENDING_CLOSE,		/**< close is pending, but not yet completed (one peer send a close request) */
	sbChan_STATE_CLOSED,			/**< channel is closed. Next thing is to do an ordinary destroy */
	sbChan_STATE_BROKEN,			/**< for some (unkonw reason) the channel does not work any longer. Must abort session. */
	sbChan_STATE_ERR_FREE_NEEDED	/**< for some (unkonw reason) the channel does not work any longer. The channel object shall be freed by the destructor (but is not yet). */
};
typedef enum sbChannelState_ sbChannelState;

struct sbMesgObject;
struct sbSessObject;
struct sbChanObject
/** The BEEP channel object. Implemented via \ref beepchannel.h and
 *  \ref beepchannel.c.
 */
{	
	srObjID OID;			/**< object ID */
	unsigned uChanNum;		/**< BEEP-assigned channel number */
	unsigned uSeqno;		/**< current seqno */
	unsigned uMsgno;		/**< current msgno */
	unsigned uTXWin;		/**< maximum transmit window */
	unsigned uTXWinLeft;	/**< remaining bytes left in current tx window */
	unsigned uRXWin;		/**< maximum receive window */
	unsigned uRXWinLeft;	/**< remaining bytes left in current rx window */
	sbSockObj* pSock;		/**< associated socket object */
	struct sbSessObject* pSess;/**< associated session */
	sbChannelState iState;	/**< channel status */
	void *pProfInstance;	/**< pointer to associated profile instance data (to be used by the profile, if needed) */
	struct sbProfObject *pProf;	/**< associated profile */
};
typedef struct sbChanObject sbChanObj;

#include "beepmessage.h"

/** 
 * Set the channel number for this channel (done by channel 0 profile).
 *
 * IMPORTANT: the channel number MUST be set only ONCE and this
 *            BEFORE any data is sent or received over the channel.
 *            The reason for this is that this method also 
 *            adds the channel to session's list of channel
 *            objects. If it would be called multiple times,
 *            multiple copies would be present in the list - which
 *            would (mildly said) cause unpredictable results).
 *
 *            Of course, we could change the behaviour to handle
 *            this case, but I do not see any good reason for this.
 *            So far, it looks like a waste of CPU cycles...
 *
 * \param iChanno [in] Channel number to be used on this channel.
 */
srRetVal sbChanSetChanno(sbChanObj *pThis, int iChanno);

/** sbChan Constructor.
 * \param pSock [in] pointer to socket to use. Can not be NULL.
 * \retval Pointer to channel object or NULL, if error occured.
 */
sbChanObj* sbChanConstruct(struct sbSessObject* pSess);

/**
 * Send a BEEP frame on this channel.
 * The message provided as parameter is sent over the channel. This
 * method here performs all necessary processing, including 
 * conversion from Mesg to (eventually multiple) frames.
 *
 * \param pMesg Pointer to the Mesg to be sent.
 *
 * \param szCmd Command to send (e.g. "ANS", "MSG", ...). Can 
 *        not be NULL.
 */
srRetVal sbChanSendMesg(sbChanObj *pThis, struct sbMesgObject* pMesg, char* pszCmd);

/**
 * Actually send the frame over the channel.
 * We need to first check if the frame is within the current Window 
 * and - if not - we need to return immediately to the caller. The
 * session layer will then handle the rest.
 *
 * For full details on the send process, see \ref architecture.c
 *
 * \param pFram [in] pointer to to-be-sent frame
 * \retval The usual values, but let's point out that
 *         SR_RET_REMAIN_WIN_TOO_SMALL must be returned if the
 *         remaining window is too small to send the frame.
 */
srRetVal sbChanActualSendFram(sbChanObj *pThis, struct sbFramObject* pFram);


/**
 * Destructor. The channel object itself will
 * be freed and can not be used any longer.
 */
void sbChanDestroy(sbChanObj* pThis);

/**
 * Update the channel status.
 *
 * \param iNewState new channel status
 */
srRetVal sbChanUpdateChannelState(sbChanObj* pThis, int iNewState);

/**
 * Assign a profile to this channel. Only one profile
 * can be assigned to a channel at one time. As such,
 * no profile is allowed to be already assigned when
 * this method is called.
 *
 * Once the profile object has been assigned to the
 * channel, the channel "owns" it. That means the caller
 * is not allowed to use it any longer or free() it.
 * The channel will free it when the channel is destroyed.
 *
 * \param pProf Profile to be assigned.
 */
srRetVal sbChanAssignProfile(sbChanObj *pThis, struct sbProfObject *pProf);

/**
 * Aborts a channel. That is, it destroys
 * the channel object itself but DOES NOT remove
 * it from the session's linked list of channels.
 */
void sbChanAbort(sbChanObj* pThis);

/**
 * Send an error response to the remote peer.
 *
 * \param uErrCode numeric error code as of RC 3080
 * \param pszErrMsg human-readable error message string. MUST
 *        NOT be NULL.
 */
srRetVal sbChanSendErrResponse(sbChanObj *pThis, unsigned uErrCode, char* pszErrMsg);

/**
 * Set the channel status to "awaiting close".
 */
srRetVal sbChanSetAwaitingClose(sbChanObj* pThis);

/**
 * Send a SEQ frame for the current channel.
 *
 * \param pChan Channel this SEQ is for.
 * \param uAckno ackno to use in the SEQ. Can not be 0.
 * \param uWindow Window to use in the SEQ. If 0, use BEEP default.
 */
srRetVal sbChanSendSEQ(sbChanObj *pThis, unsigned uAckno, unsigned uWindow);

/**
 * Send an OK on the associated channel. This is
 * just a shortcut to save some typing.
 */
srRetVal sbChanSendOK(sbChanObj*pThis, void (*OnFrameDestroy)(struct sbFramObject*), void* pUsr);

/**
 * Set channel state to closed.
 */
srRetVal sbChanSetChanClosed(sbChanObj* pThis);

#endif
