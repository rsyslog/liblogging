/*! \file clntprof-3195raw.h
 *  \brief The client profile for RFC 3195 raw.
 *
 * The prefix for this "object" is psrr which stands for
 * *P*rofile *S*yslog *R*eliable *Raw*. This file works in
 * conjunction with \ref lstnprof-3195raw.c and shares
 * its namespace.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-09-04
 *          coding begun
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
#ifndef __LIB3195_CLNTPROF_3195RAW_H_INCLUDED__
#define __LIB3195_CLNTPROF_3195RAW_H_INCLUDED__ 1
#define sbPSSRCHECKVALIDOBJECT(x) {assert(x != NULL); assert(x->OID == OIDsbPSSR);}

/**
 * The RAW profile object used by the client profile.
 */
struct sbPSSRObject_
{
	srObjID OID;					/**< object ID */
	SBansno	uAnsno;					/**< ansno  */
	SBmsgno uMsgno4raw;				/**< msgno to be used for rfc3195/RAW messages */
};
typedef struct sbPSSRObject_ sbPSSRObj;

/**
 * Send a message to the remote peer.
 */
srRetVal sbPSSRClntSendMsg(sbChanObj* pChan, char* szLogmsg);

/**
 * Handler to be called when a new channel is
 * established.
 *
 * This handler is  the first to be called. So it
 * will also create the instance's data object, at least
 * for those profiles, that need such.
 *
 * There is not much to do for RFC 3195/RAW ;)
 */
srRetVal sbPSSRClntOpenLogChan(sbChanObj *pChan);

/**
 * Handler to be called when a channel is to be closed.
 */
srRetVal sbPSSRCOnClntCloseLogChan(sbChanObj *pChan);

/** 
 * Handler to send a srSLMGObj to the remote peer. This
 * is the preferred way to send things.
 *
 * For -RAW, it is very easy - it just needs to call the
 * SendMesg method with the RAW string. There is nothing
 * that -RAW adds to this... ;)
 */
srRetVal sbPSSRClntSendSLMG(struct sbChanObject* pChan, struct srSLMGObject *pSLMG);

#endif
