/* The stdlog main header file
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

#ifndef LIBLOGGING_STDLOG_H_INCLUDED
#define LIBLOGGING_STDLOG_H_INCLUDED
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

/* options for stdlog_open() call */
#define STDLOG_SIGSAFE 1	/* enforce signal-safe implementation */
#define STDLOG_USE_DFLT_OPTS ((int)0x80000000)	/* use default options */

/* traditional syslog facility codes */
#define	STDLOG_KERN	0	/* kernel messages */
#define	STDLOG_USER	1	/* random user-level messages */
#define	STDLOG_MAIL	2	/* mail system */
#define	STDLOG_DAEMON	3	/* system daemons */
#define	STDLOG_AUTH	4	/* security/authorization messages */
#define	STDLOG_SYSLOG	5	/* messages generated internally by syslogd */
#define	STDLOG_LPR	6	/* line printer subsystem */
#define	STDLOG_NEWS	7	/* network news subsystem */
#define	STDLOG_UUCP	8	/* UUCP subsystem */
#define	STDLOG_CRON	9	/* clock daemon */
#define	STDLOG_AUTHPRIV	10	/* security/authorization messages (private) */
#define	STDLOG_FTP	11	/* ftp daemon */
/* other codes through 15 reserved for system use */
#define	STDLOG_LOCAL0	16	/* reserved for local use */
#define	STDLOG_LOCAL1	17	/* reserved for local use */
#define	STDLOG_LOCAL2	18	/* reserved for local use */
#define	STDLOG_LOCAL3	19	/* reserved for local use */
#define	STDLOG_LOCAL4	20	/* reserved for local use */
#define	STDLOG_LOCAL5	21	/* reserved for local use */
#define	STDLOG_LOCAL6	22	/* reserved for local use */
#define	STDLOG_LOCAL7	23	/* reserved for local use */

/* traditional syslog priorities */
#define	STDLOG_EMERG	0	/* system is unusable */
#define	STDLOG_ALERT	1	/* action must be taken immediately */
#define	STDLOG_CRIT	2	/* critical conditions */
#define	STDLOG_ERR	3	/* error conditions */
#define	STDLOG_WARNING	4	/* warning conditions */
#define	STDLOG_NOTICE	5	/* normal but significant condition */
#define	STDLOG_INFO	6	/* informational */
#define	STDLOG_DEBUG	7	/* debug-level messages */

typedef struct stdlog_channel *stdlog_channel_t;

const char *stdlog_version(void);
size_t stdlog_get_msgbuf_size(void);
const char *stdlog_get_dflt_chanspec(void);
int stdlog_init(uint32_t options);
void stdlog_deinit(void);
stdlog_channel_t stdlog_open(const char *ident, const int option, const int facility, const char *channelspec);
void stdlog_close(stdlog_channel_t channel);
int stdlog_log(stdlog_channel_t channel, const int severity, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
int stdlog_log_b(stdlog_channel_t ch, const int severity, char *wrkbuf, const size_t buflen, const char *fmt, ...);
int stdlog_vlog(stdlog_channel_t ch, const int severity, const char *fmt, va_list ap);
int stdlog_vlog_b(stdlog_channel_t ch, const int severity, char *__restrict__ const wrkbuf, const size_t buflen, const char *fmt, va_list ap);

#endif /* multi-include protection */
