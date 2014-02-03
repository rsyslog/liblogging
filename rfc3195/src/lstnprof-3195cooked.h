/*! \file lstnprof-3195cooked.h
 *  \brief The 3195 COOKED listener profile.
 *
 * Listener profiles are specific in that they do not support their
 * own full blown objects but only the callback code.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-09-05
 *          file creatd, but coding not yet begun
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
#ifndef __LIB3195_LISTNPROF_3195COOKED_H_INCLUDED__
#define __LIB3195_LISTNPROF_3195COOKED_H_INCLUDED__ 1

struct sbMesgObject;

srRetVal psrcOnChanCreate(struct sbProfObject *pThis, struct sbSessObject* pSess, struct sbChanObject* pChan);
srRetVal psrcOnMesgRecv(struct sbProfObject *pThis, int* pbAbort, struct sbSessObject* pSess, struct sbChanObject* pChan, struct sbMesgObject *pMesg);


#endif
