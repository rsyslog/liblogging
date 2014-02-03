/*! \file settings.h
 *  \brief configuration settings for liblogging.
 *
 * This file includes all global defines as well as configuration settings.
 * If you intend the lib to another environment, THIS is the place
 * to add portability macros.
 *
 * \author  Rainer Gerhards <rgerhards@adiscon.com>
 * \date    2003-08-04
 *          0.1 version created.
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

#ifndef __LIB3195_CONFIG_H_INCLUDED__
#define __LIB3195_CONFIG_H_INCLUDED__ 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** 
 * Default TCP Windows size as defined in RFC 3081. We may also define
 * a value that suites us better...
 */
#define BEEP_DEFAULT_WINDOWSIZE 4096

/**
 * Maximum size of a single BEEP frame.
 */
#define BEEPFRAMEMAX BEEP_DEFAULT_WINDOWSIZE

/**
 * Maximum size of the socket layer receive buffer.
 * \note A too-small buffer can cause performance 
 * degradation but can not break the implementation. This
 * is just a buffer used when reading incoming data. The
 * upper layers read byte by byte and the lower layer
 * re-reads data when the buffer is empty. Anyhow, it is
 * a good idea to leave it as default.
 */
#define SOCKETMAXINBUFSIZE BEEP_DEFAULT_WINDOWSIZE


/**
 * Size of dynamic string buffer growth. The dynamic string
 * buffer object allocates memory in STRINGBUF_ALLOC_INCREMENT
 * increments. This is to avoid too many malloc calls. If 
 * this number is too large, probably time and memory is
 * wasted. However, if it is too low, too many allocations
 * happen which in turn, too, results in performance degradation.
 * It should not be set to below 128 bytes. The default value
 * of 1024 is a good estimate and works well with most
 * processor page sizes. You should only change this value
 * after you have thought a while over the implications.
 */
#define STRINGBUF_ALLOC_INCREMENT 1024

/**
 * This is an important setting for the dynamic string
 * object. If set to 1, the dynamic string object trims
 * the string to its maximum size once string processing
 * is finished and the resulting string is returned to
 * the caller. To do so, it needs to allocate a new 
 * buffer, copy the string over to that new buffer and
 * then free the old one. If set to 0, the old (existing)
 * buffer is simply returned. This will result in more
 * memory being used (because the buffer most probably is
 * a bit oversized), but it safes the extra processing time
 * for the additional alloc, free and STRING COPY. We
 * recommend using mode 0 for all environments, except
 * when memory ressources are really constraint, in which
 * case mode 1 should be selected. Do not use mode 1
 * unless you have a very good reason to do so!
 */
#define STRINGBUF_TRIM_ALLOCSIZE 0

/**
 * Enables debug aids if defined to 1, disables them
 * otherwise.
 */
#define DEBUGLEVEL 1

/**
 * This is a security relevant setting. It specifies how
 * channel 0 error messages are sent. We support full-fledged
 * error messages which make diagnosing the error cause
 * (hopefully) relatively easy. On the other hand, though,
 * they may be abused to find something out about the inner
 * workings or state of the server process.
 * This setting allows to tune the level of information
 * provided to the remote peer:
 * 0 - totally unspecifc error message, all replies will
 *     be 550 "error occured".
 * 1 - all error codes are reported as they are provided
 *     EXCEPT for the 451s (which reaveal internal errors).
 *     451s are reported as 550 "error occured"
 * 2 - all error codes (including 451s) are sent in full
 */
#define SECURITY_PEER_ERRREPORT_LEVEL 1

/* ######################################################################### *
 * # Feature Select Macros - these macros turn library features on or off. # *
 * # The library modules have been choosen with care so that in most cases # *
 * # only those code & data actually needed is linked into the program.    # *
 * # However, there are some instances where code is only needed for spe-  # *
 * # cific features and if such features are not used can be removed. Such # *
 * # fine-tuning can be done with the below macros. We highly suggest to   # *
 * # change settings only if you EXACTLY know WHAT your are doing AND WHY  # *
 * # you are doing it - otherwise, you'll most probably end up with linker # *
 * # errors at least...                                                    # *
 * #                                                                       # *
 * # In general, a feature is enabled if it is defined to 1 and disabled   # *
 * # if it is set to 0.                                                    # *
 * ######################################################################### */

/**
 * Should the listener features be provided?
 */
#define FEATURE_LISTENER 1

/**
 * Should UDP (RFC 3164) features be provided?
 */
#define FEATURE_UDP 1

/**
 * Should Unix Domain Sockets feature be provided?
 */
#define FEATURE_UNIX_DOMAIN_SOCKETS 1

/**
 * Should the COOKED profile be provided?
 * If set to 1, COOKED will be implemented for the
 * client and, if FEATURE_LISTNER is set, for the
 * listener part, too.
 */
#define FEATURE_COOKED 1

/**
 * Should the syslog message API be provided?
 * Please note that some limited functionality of
 * the API is provided in any case if FEATURE_LISTENER
 * is turned on. However, even in that case no parsing
 * will take place if FEATURE_MSGAPI is not turned on.
 */
#define FEATURE_MSGAPI 1

/* ######################################################################### *
 * #                 PORTABILITY MACROS FROM HERE ON                       # *
 * ######################################################################### */

/* As it looks, different compilers have different predefined
 * defines for the OS environment. Not really nice :-(
 * So we define our own OS defines and use them consistently.
 */
#ifdef WIN32
#	define SROS_WIN32
#endif

#ifdef __win32
#	define SROS_WIN32
#endif

#ifdef SROS_Solaris_
#   define SROS_Solaris
#endif


/* ######################################################################### *
 * # Now all environment defines are made, so let's get down to the meat ;)# *
 * ######################################################################### */

#ifndef TRUE
#	define TRUE 1
#	define FALSE 0
#endif

#ifdef SROS_WIN32
#	define SLEEP(x) Sleep(x)
#	define SR_SOCKET	SOCKET
#	define SNPRINTF		_snprintf
	/* for obvious reasons, we define FEATURE_UNIX_DOMAIN_SOCKETS to 0 under win32... */
#	undef FEATURE_UNIX_DOMAIN_SOCKETS
#	define FEATURE_UNIX_DOMAIN_SOCKETS 0
#else
#	define SLEEP(x) sleep(x)
#	define SR_SOCKET	int
#	define INVALID_SOCKET 0	/**< \todo verify this value is indeed invalid under *nix */
#	define SNPRINTF snprintf
#endif


/* ######################################################################### *
 * #                       END PORTABILITY MACROS                          # *
 * ######################################################################### */

#endif
