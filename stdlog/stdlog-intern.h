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

#define __STDLOG_MSGBUF_SIZE 4096
#ifndef STDLOG_INTERN_H_INCLUDED
#define STDLOG_INTERN_H_INCLUDED
struct stdlog_channel {
	const char *spec;
	const char *ident;
	int32_t options;
	int facility;
	char *fmtbuf;
	int (*f_vsnprintf)(char *str, size_t size, const char *fmt, va_list ap);
	struct {
		void (*init)(stdlog_channel_t ch); /* initialize driver */
		void (*open)(stdlog_channel_t ch);
		void (*close)(stdlog_channel_t ch);
		int (*log)(stdlog_channel_t ch, const int severity, const char *fmt, va_list ap, char *wrkbuf, const size_t buflen);
	} drvr;
	union {
		struct {
			char *sockname;
			int sock;
			struct sockaddr_un addr;
		} uxs;	/* unix socket (including syslog) */
		struct {
			int fd;
			char *name;
		} file;
	} d;	/* driver-specific data */
};

/* A useful macro for adding single chars during string building. It ensures
 * that the target buffer is not overrun. The index idx must be an lvalue and
 * must point to the location where the character is to be inserted. If there
 * is sufficient space left, to char is inserted and the index incremented.
 * Otherwise, no updates happen.
 */
#define __STDLOG_STRBUILD_ADD_CHAR(buf, lenbuf, idx, c) \
	if(idx < (int) (lenbuf)) { \
		buf[(idx)++] = c; \
	}

int __stdlog_formatTimestamp3164(const struct tm *const tm, char *const  buf);
struct tm * __stdlog_timesub(const time_t * timep, const long offset, struct tm *tmp);

void __stdlog_set_uxs_drvr(stdlog_channel_t ch);
void __stdlog_set_jrnl_drvr(stdlog_channel_t ch);
void __stdlog_set_file_drvr(stdlog_channel_t ch);

/* formatter "library" routines */
void __stdlog_fmt_print_int (char *__restrict__ const buf, const size_t lenbuf, int *idx, int64_t nbr);
void __stdlog_fmt_print_str (char *__restrict__ const buf, const size_t lenbuf, int *__restrict__ const idx, const char *const str);
int __stdlog_sigsafe_printf(char *buf, const size_t lenbuf, const char *fmt, va_list ap);
void __stdlog_sigsafe_memcpy(void *dest, const void *src, size_t n);
int __stdlog_wrapper_vsnprintf(char *buf, size_t lenbuf, const char *fmt, va_list ap);

#endif /* STDLOG_INTERN_H_INCLUDED */
