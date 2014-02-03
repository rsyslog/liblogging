/*! \file beepframe.h
 *  \brief The BEEP frame object
 * 
 *  \note There are frames not belonging to messages! Curently,
 *  this is the case for SEQ frames defined in RFC 3081.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
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
#ifndef __LIB3195_BEEPFRAME_H_INCLUDED__
#define __LIB3195_BEEPFRAME_H_INCLUDED__ 1

#include "beepmessage.h"
#include "beepchannel.h"
#define sbFramCHECKVALIDOBJECT(x) {assert(x != NULL); assert(x->OID == OIDsbFram);}

/** Session status */
enum sbFramState_
{
	/* RX flags */
	sbFRAMSTATE_WAITING_HDR1,	/**< waiting for the first HDR character */
	sbFRAMSTATE_WAITING_HDR2,	/**< waiting for the second HDR character */
	sbFRAMSTATE_WAITING_HDR3,	/**< waiting for the third HDR character */
	sbFRAMSTATE_WAITING_SP_CHAN,/**< waiting for the SP before channo */
	sbFRAMSTATE_IN_CHAN,		/**< reading channo */
	sbFRAMSTATE_WAITING_SP_MSGNO,/**< now the space before the next header */
	sbFRAMSTATE_IN_MSGNO,		/**< and the next (numeric) header */
	sbFRAMSTATE_WAITING_SP_MORE,/**< now the space before the next header */
	sbFRAMSTATE_IN_MORE,		/**< and the next (char) header */
	sbFRAMSTATE_WAITING_SP_SEQNO,/**< now the space before the next header */
	sbFRAMSTATE_IN_SEQNO,		/**< and the next (numeric) header */
	sbFRAMSTATE_WAITING_SP_SIZE,/**< now the space before the next header */
	sbFRAMSTATE_IN_SIZE,		/**< and the next (numeric) header */
	sbFRAMSTATE_WAITING_SP_ANSNO,/**< now the space before the next header */
	sbFRAMSTATE_IN_ANSNO,		/**< and the next (numeric) header */
	sbFRAMSTATE_WAITING_SP_ACKNO,/**< now the space before the next header */
	sbFRAMSTATE_IN_ACKNO,		/**< and the next (char) header */
	sbFRAMSTATE_WAITING_SP_WINDOW,/**< now the space before the next header */
	sbFRAMSTATE_IN_WINDOW,		/**< and the next (numeric) header */
	sbFRAMSTATE_WAITING_HDRCR,	/**< awaiting the HDR's CR */
	sbFRAMSTATE_WAITING_HDRLF,	/**< awaiting the HDR's LF */
	sbFRAMSTATE_IN_PAYLOAD,		/**< reading payload area */
	sbFRAMSTATE_WAITING_END1,	/**< waiting for the 1st HDR character */
	sbFRAMSTATE_WAITING_END2,	/**< waiting for the 2nd HDR character */
	sbFRAMSTATE_WAITING_END3,	/**< waiting for the 3rd HDR character */
	sbFRAMSTATE_WAITING_END4,	/**< waiting for the 4th HDR character */
	sbFRAMSTATE_WAITING_END5,	/**< waiting for the 5th HDR character */
	sbFRAMSTATE_RECEIVED,		/**< frame is fully received and ready for processing*/

	/* TX flags */
	sbFRAMSTATE_BEING_BUILD,	/**< frame is currently being constructed */
	sbFRAMSTATE_READY_TO_SEND,	/**< frame is constructed and ready to send */
	sbFRAMSTATE_SENDING,		/**< frame is currently being send */
	sbFRAMSTATE_SENT,			/**< frame has been send and can be discarded */

	sbFRAMSTATE_UNKNOWN = 0		/**< should never happen. Set after initialization. */
};
typedef enum sbFramState_ sbFramState;

struct sbFramObject
/** The BEEP frame object. Implemented via \ref beepframe.h and
 *  \ref beepframe.c.
 */
{	
	srObjID	OID;				/**< Object ID - to make code (more ;)) bulletproof */
	struct sMesgObject* pMsg;	/**< pointer to the message this frame belongs to. May be NULL
								*    if the frame does NOT belong to a message (SEQ frame!)
								*/
	sbFramState iState;			/**< status of this frame */
	char* szRawBuf;				/**< pointer to the complete frame text */
	int iFrameLen;				/**< size (in Bytes) of the complete frame */
	BEEPHdrID idHdr;			/**< the header id of this frame */
	SBackno uAckno;				/**< RFC 3081 ackno */
	SBwindow uWindow;			/**< RFC 3081 window */
	SBchannel uChannel;			/**< RFC 3080 channel */
	SBmsgno uMsgno;				/**< RFC 3080 msgno */
	SBseqno uSeqno;				/**< RFC 3080 seqno */
	SBsize uSize;				/**< RFC 3080 size */
	char cMore;					/**< RFC 3080 more */
	SBansno uAnsno;				/**< RFC 3080 ansno */
#if FEATURE_LISTENER == 1
	unsigned uBytesSend;		/**< number of bytes already send */
	struct sbChanObject *pChan;	/**< channel to send over (shortcut to save performance) */
	struct sbStrBObject *pStrBuf;/**< work buffer for receiving frames */
	unsigned uToReceive;		/**< number of payload characters (still) to be recieved */
	void (*OnFramDestroy)(struct sbFramObject*);	/**< callback to be called when frame is destroyed */
	void* pUsr;					/**< user pointer to be passed to callback OnFramDestroy() */
#endif
};
typedef struct sbFramObject sbFramObj;

char* sbFramGetFrame(sbFramObj* pThis); /**< return frame content */
int sbFramGetFrameLen(sbFramObj* pThis); /**< return length of frame */
BEEPHdrID sbFramGetHdrID(sbFramObj *pThis); /**<  Returns the header id of this BEEP frame. */

srRetVal sbFramDestroy(sbFramObj *pThis); /**< destroy a frame object */

/** Create a frame from a Message.
 *  With the current implementation, there is
 *  a one-to-one relationship between fram and mesg, so 
 *  this is faily simple ;)
 *
 *  \param pChan [in/out] Channel object that this frame
 *         will be sent on. Is updated with new SEQ number.
 *
 *  \param pMesg [in] Pointer to the Mesg to be converted. This
 *                    buffer must not be modified by Mesg after
 *                    CreateFramFromMesg has been called!
 *
 * \param pszCmd [in] HDR-Command ("ANS", "MSG", ...) to be used.
 *
 * \param uAnso [in] if the HDR-Command is "ANS", this is the ansno
 *                   to be used. Ignored with any other HDR-Command.
 *
 * \retval Pointer to a newly created Frame object or NULL if
 *         an error occured.
 */
sbFramObj* sbFramCreateFramFromMesg(sbChanObj* pChan, sbMesgObj* pMesg, char* pszCmd, SBansno uAnsno);

/**
 * Actually receive a frame off the tcp stream.
 * Be sure to read \ref architecture.c for a full
 * explanation of what ActualRecvFram and its associated
 * methods do in an overall context.
 *
 * In detail, it receives a frame off the wire and
 * returns the resulting farme object to its caller.
 * It detects different frame formats (like SEQ frames).
 *
 * This is a BLOCKING method call. It blocks until the
 * frame is complete or a timeout occurs.
 *
 * \param pSess pointer to session object. This session
 *        is to be used to receive the frame.
 *
 * \retval pointer to the received frame object or
 *         NULL, if an error occured.
 */
sbFramObj* sbFramActualRecvFram(sbSessObj *pSess);

/**
 * Logically receive a frame. This method uses the session object
 * to do the physical receive. See \ref architecture.c for a 
 * full description of this interaction.
 *
 * This is just a slim wrapper to hide the implementation details.
 *
 * \param pChan [in/out] associated channel
 * \retval pointer to new frame object or NULL, if in error.
 */
sbFramObj* sbFramRecvFram(sbChanObj* pChan);

/**
 * Send a frame on a channel (logical). This method does
 * not directly put the data on the wire but calls the
 * session object to initiate this. This sequence allows
 * for limited multiprocessing inside the stack. Please
 * see \ref architecture.c for a full description.
 *
 * This is just a very thin layer in this release.
 *
 * \param pChan the associated channel (where the frame
 *              is to be sent).
 */
srRetVal sbFramSendFram(sbFramObj* pThis, sbChanObj*pChan);

/**
 * Get the header ID for a given BEEP command header. This
 * is implemented in the sbFram object as sbFram ultimately
 * creaetes the frames and thus the IDs. One could argue
 * if a better place would be some util helper...
 *
 * \param psz [in] pointer to a BEEP header command ("MSG", ...)
 * \retval matching header id
 */
BEEPHdrID sbFramHdrID(char* szCmd);

/**
 * Construct a frame. This builds the memory structure, only
 * but does not do anything to actually populate it.
 */
srRetVal sbFramConstruct(sbFramObj **ppThis);

/**
 * Construct a SEQ frame for a given channel with a
 * given ackno.
 *
 * \param pChan Channel this SEQ is for.
 * \param uAckno ackno to use in the SEQ. Can not be 0.
 * \param uWindow Window to use in the SEQ. If 0, use BEEP default.
 */
srRetVal sbFramCreateSEQFram(sbFramObj **ppThis, sbChanObj* pChan, SBackno uAckno, SBwindow uWindow);

/** 
 * Register a callback handler to be called when the frame is sent and ready
 * to be destroyed. If there was already a handler set, it is REPLACED
 * with the new one. There is no "calling-queue" of multiple handlers.
 *
 * \param OnFramDestroy Pointer to Callback. Must not be NULL.
 * \param Pointer to user pointer (may be NULL)
 */
srRetVal sbFramSetOnDestroyEvent(sbFramObj *pThis, void (*OnFramDestroy)(struct sbFramObject *), void *pUsr);

#endif
