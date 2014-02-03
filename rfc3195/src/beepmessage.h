/*! \file beepmessage.h
 *  \brief The BEEP Message Object
 *
 *  The current implemantation does NOT support fragementation. As
 *  such, all calls in this object are directly relayed through to
 *  the sbFram object.
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
#ifndef __LIB3195_BEEPMESSAGE_H_INCLUDED__
#define __LIB3195_BEEPMESSAGE_H_INCLUDED__ 1

#include "beepchannel.h"
#include "beepsession.h"
#define sbMesgCHECKVALIDOBJECT(x) {assert(x != NULL); assert(x->OID == OIDsbMesg);}

struct sbFramObject;
struct sbMesgObject
/** The BEEP message object. Implemented via \ref beepmessage.h and
 *  \ref beepmessage.c.
 */
{	
	srObjID	OID;			/**< Object ID - to make code (more ;)) bulletproof */
	BEEPHdrID idHdr;		/**< the header id of this frame */
	SBmsgno	uMsgno;			/**< the message number of this message (from header) */
	SBseqno uSeqno;			/**< RFC 3080 seqno (from header) */
	SBseqno uNxtSeqno;		/**< expected next seqno on the channel (for RFC 3081 SEQ) */
	char* szRawBuf;			/**< pointer to the complete message text */
	char* szMIMEHdr;		/**< the message's MIME header (NULL, if no headers were provided) */
	char* szActualPayload;	/**< the *actual* payload, that is message without MIME headers
							 *    and without the CRLF MIME header separator. 
							 *	  szActualPayload POINTS DIRECTLY INTO THE RAW MESSAGE
							 *    BUFFER. This is for performance reasons. As it is no
							 *    buffer into its own right, it MUST NEVER be free()ed!
							 */
	int bRawDirty;			/**< TRUE, if szRawBuf does not contain a proper full
							 *   representation of the message. FALSE otherwise. This is
							 *   used when generating messages. It may improve performance
							 *   if we e.g. add MIME headers first before assemblin the
							 *   final message.
							 *
							 *   NOT CURRENTLY USED.
							 */
	int iPayloadSize;		/**< size (in bytes) of the actual payload */
	int iMIMEHdrSize;		/**< size (in bytes) of the MIME headers */
	int iOverallSize;		/**< size (in bytes) of the whole message */
};
typedef struct sbMesgObject sbMesgObj;

char* sbMesgGetRawBuf(sbMesgObj* pThis);	/**< return the raw buffer object */
int sbMesgGetMIMEHdrSize(sbMesgObj* pThis);	/**< return the size of the MIME Header (in bytes) */
int sbMesgGetPayloadSize(sbMesgObj* pThis);	/**< return the size of the actual payload (in bytes) */
int sbMesgGetOverallSize(sbMesgObj* pThis);	/**< return the overall message size (in bytes) */

/** Constructor to create a Mesg based on provided frame.
 *
 * \param psbFram [in] frame to be used for message creation.
 *
 * \retval Pointer to new message object or NULL if an error
 *         occured.
 */
sbMesgObj* sbMesgConstrFromFrame(struct sbFramObject* psbFram);

/** Constructor to create a Mesg based on provided values. with no MIME headers.
 *
 * \param szMIMEHdr [in] buffer to MIME headers. Is copied to
 *        private buffer space. May be NULL, in which case no
 *        MIME headers are present.
 * \param szPayload [in] buffer to payload data. Is copied to
 *        private buffer. May be NULL, in which case an empty
 *        payload is created after the MIME header.
 *
 * \retval Pointer to new message object or NULL if an error
 *         occured.
 */
sbMesgObj* sbMesgConstruct(char* pszMIMEHdr, char *pszPayload);

/**
 * Receive a message from a given channel.
 * As we do not currently support fragmentation, this
 * method simply reads a single fram off the socket
 * and uses that frame to construct the message.
 * 
 * \retval Pointer to newly constructed MesgObj or NULL
 *         if an error occured.
 */
sbMesgObj* sbMesgRecvMesg(sbChanObj*);


/**
 * Extract MIME header and body from a given message buffer.
 * Even when no MIME Header is present, there still must
 * be a header terminatiing CRLF.
 *
 * To the aid of supporters looking at this code: SDSC
 * syslog 1.0.0 has a bug, where it does not show any message
 * if there is the mime header in it. This has been fixed
 * in SDSC 1.0.1. So if someone has this issue with 1.0.0,
 * the only solution is to upgrade SDSC - there is nothing
 * wrong in our code (see SDSC syslog on sourceforge.net if
 * you don't trust this comment ;)).
 *
 * \param pszInBuf [in] buffer to process
 * \param iInBufLen [in] Length of input buffer in bytes
 * \param pszMIMEHdr [out] pointer to the MIME header
 *                         NULL, if no header is present
 * \param PSZPayload [out] pointer to the payload
 *                         NULL, if no payload is present
 * \retval Pointer to newly constructed MesgObj or NULL
 *         if an error occured.
 */
srRetVal sbMIMEExtract(char *pszInBuf, int iInBufLen, char **pszMIMEHdr, char** pszPayload);


/**
 * Send a message over the specified channel object.
 * \param pChan [in] associated channel. Channel housekeeping 
 *                   data is updated.
 *
 * \param pszCmd [in] HDR-Command to be used, e.g. "MSG", "ANS", ...
 * \param uAnsno [in] ansno to use if HDR command is "ANS", ignored
 *                    otherwise. We recommend setting it to 0 if not used.
 * \param OnFramDestroy If non-NULL, callback to be called when the frame is destroyed.
 * \param pUsr If onFramDestroy is non-NULL, the user pointer to be provided to the
 *             callback. Otherwise ignored.
 */
srRetVal sbMesgSendMesgWithCallback(sbMesgObj* pThis, sbChanObj* pChan, char *pszCmd, SBansno uAnsno,  void (*OnFramDestroy)(struct sbFramObject*), void* pUsr);

/**
 * The same as \ref sbMesgSendMesgWithCallback, except
 * that no callback is provided.
 */
srRetVal sbMesgSendMesg(sbMesgObj* pThis, sbChanObj* pChan, char *pszCmd, SBansno);

/**
 * Destructor. Does not return any value as the primary function
 * to be used is free() which itself does not return any status.
 */
void sbMesgDestroy(sbMesgObj *pThis);

#endif
