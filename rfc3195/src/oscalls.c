/**\file oscalls.c
 * \brief Wrapper "class" for all kinds of utility OS API
 * calls.
 *
 * This file implements includes all the OS API calls which do
 * not make up an object in their own right.
 *
 * Its primary purpose is to easy porting.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-09-02
 *          Coding begun.
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

#include "settings.h"
#include "liblogging.h"
#include "oscalls.h"
#include "assert.h"

/* Now include the lower level driver so that we have
 * private (static) methods available during the rest of
 * the file.
 *
 * We also define some prototpyes of private
 * members that follow later in this file (no way
 * around this...).
 */

#ifdef SROS_WIN32
#	include "oscallsWin32.c"
#else
#	include "oscallsUnix.c"
#endif


/* ################################################################# *
 * private members                                                   *
 * ################################################################# */

/* As this is not a "real" object, there won't be any private
 * members in this file.
 */

/* ################################################################# *
 * public members                                                    *
 * ################################################################# */

