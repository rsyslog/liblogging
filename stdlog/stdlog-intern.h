/* The stdlog private definitions file.
 *
 * Copyright (C) 2014 Adiscon GmbH
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ADISCON AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ADISCON OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <time.h>
#include <sys/un.h>
#include "stdlog.h"

#ifndef STDLOG_INTERN_H_INCLUDED
#define STDLOG_INTERN_H_INCLUDED
struct stdlog_channel {
	const char *spec;
	const char *ident;
	int options;
	int facility;
	char *fmtbuf;
	union {
		struct {
			int sock;
			struct sockaddr_un addr;
		} uxs;	/* unix socket (including syslog) */
	} d;	/* driver-specific data */
	size_t lenmsg;	/* size of formatted msg in msgbuf */
	char msgbuf[4096]; /* message buffer (TODO: dynmic size!) */
};


int __stdlog_formatTimestamp3164(const struct tm *const tm, char *const  buf);
struct tm * __stdlog_timesub(const time_t * timep, const long offset, struct tm *tmp);

/* uxsock driver */
void __stdlog_uxs_log(stdlog_channel_t ch);

/* systemd journal driver */
void __stdlog_jrnl_log(stdlog_channel_t ch, const int severity);

#endif /* STDLOG_INTERN_H_INCLUDED */
