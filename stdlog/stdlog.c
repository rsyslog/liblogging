/* The stdlog main file
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
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <syslog.h>
#include <errno.h>
#include "stdlog-intern.h"


static stdlog_channel_t dflt_channel = NULL;
static char *dflt_chanspec = NULL;
static int32_t dflt_options = 0;


/* can be called before any other library call. If so, initializes
 * some library structures.
 * NOTE: this API may change in the final version
 * returns 0 on success, -1 on error with errno set
 */
int
stdlog_init(uint32_t options)
{
	char *chanspec;

	if (dflt_channel != NULL) {
		errno = EINVAL;
		return -1;
	}
	if ((options & STDLOG_USE_DFLT_OPTS) != 0) {
		errno = EINVAL;
		return -1;
	}

	dflt_options = options;
	chanspec = getenv("LIBLOGGING_STDLOG_DFLT_LOG_CHANNEL");
	if (chanspec == NULL)
		chanspec = "syslog:";
	if ((dflt_chanspec = strdup(chanspec)) == NULL)
		return -1;

	if((dflt_channel = 
	      stdlog_open("liblogging-stdlog", dflt_options, STDLOG_LOCAL7, NULL)) == NULL)
		return -1;

	return 0;
}

/* may be called to free stdlog ressources
 * (e.g. to prevent valgrind mem leaks at shutdown)
 */
void
stdlog_deinit (void)
{
	free(dflt_chanspec);
}

const char *
stdlog_version(void)
{
	return VERSION;
}

size_t
stdlog_get_msgbuf_size(void)
{
	return __STDLOG_MSGBUF_SIZE;
}

const char *
stdlog_get_dflt_chanspec(void)
{
	return dflt_chanspec;
}


/* interprets a driver chanspec and sets the channel accordingly
 */
static int
__stdlog_set_driver(stdlog_channel_t ch, const char *__restrict__ chanspec)
{
	if (chanspec == NULL)
		chanspec = dflt_chanspec;

	if((ch->spec = strdup(chanspec)) == NULL) {
		errno = ENOMEM;
		return -1;
	}

	if (!strncmp(chanspec, "file:", 5))
		__stdlog_set_file_drvr(ch);
#	ifdef ENABLE_JOURNAL
	else if (!strcmp(chanspec, "journal:"))
		__stdlog_set_jrnl_drvr(ch);
#	endif
	else if (!strncmp(chanspec, "uxsock:", 7))
		__stdlog_set_uxs_drvr(ch);
	else
		__stdlog_set_uxs_drvr(ch);
	return 0;
}

stdlog_channel_t
stdlog_open(const char *ident, const int option, const int facility, const char *chanspec)
{
	stdlog_channel_t ch;

	if (   (option == STDLOG_USE_DFLT_OPTS && (option & ~STDLOG_USE_DFLT_OPTS) != 0)
	    || (facility < 0 || facility > STDLOG_LOCAL7) ) {
		ch = NULL;
		errno = EINVAL;
		goto done;
	}
	if((ch = calloc(1, sizeof(struct stdlog_channel))) == NULL) {
		errno = ENOMEM;
		goto done;
	}
	if((ch->ident = strdup(ident)) == NULL) {
		free(ch);
		ch = NULL;
		errno = ENOMEM;
		goto done;
	}
	ch->options = (option == STDLOG_USE_DFLT_OPTS) ? dflt_options : option;
	ch->facility = facility;

	/* formatting driver selection */
	ch->f_vsnprintf = (ch->options & STDLOG_SIGSAFE)
	                    ? __stdlog_sigsafe_printf : __stdlog_wrapper_vsnprintf;

	/* output driver selection */
	if(__stdlog_set_driver(ch, chanspec) != 0) {
		int errnosv = errno;
		free((char*)ch->ident);
		free((char*)ch->spec);
		free(ch);
		ch = NULL;
		errno = errnosv;
		goto done;
	}

	ch->drvr.init(ch);
done:
	return ch;
}

void
stdlog_close(stdlog_channel_t ch)
{
	ch->drvr.close(ch);
	free(ch);
}

/* the following macro is common code for the two stdlog_logXX()
 * functions.
 */
#define STDLOG_LOG_READY_CHANNEL \
	if(severity < 0 || severity > 7) { \
		r = -1; \
		goto done; \
	} \
	if(ch == NULL) { \
		if (dflt_channel == NULL) \
			if((r = stdlog_init(0)) != 0) \
				goto done; \
		ch = dflt_channel; \
	}

/* Log a message to the specified channel. If channel is NULL,
 * use the default channel (which always exists).
 * Returns 0 on success or a standard (negative) error code.
 * Otherwise the semantics are equivalent to syslog().
 */
int
stdlog_vlog(stdlog_channel_t ch,
	const int severity, const char *fmt, va_list ap)
{
	int r = 0;
	char wrkbuf[__STDLOG_MSGBUF_SIZE];

	STDLOG_LOG_READY_CHANNEL
	r = ch->drvr.log(ch, severity, fmt, ap, wrkbuf, sizeof(wrkbuf));
done:	return r;
}

/* Same as stdlog_vlog() except that a buffer can be provided
 */
int
stdlog_vlog_b(stdlog_channel_t ch, const int severity,
	char *__restrict__ const wrkbuf, const size_t buflen,
	const char *fmt, va_list ap)
{
	int r = 0;

	STDLOG_LOG_READY_CHANNEL
	r = ch->drvr.log(ch, severity, fmt, ap, wrkbuf, buflen);
done:	return r;
}

/* Same as stdlog_vlog(), except that it takes multiple arguments.
 */
int
stdlog_log(stdlog_channel_t ch,
	const int severity, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	return stdlog_vlog(ch, severity, fmt, ap);
}

/* Same as stdlog_vlog_b(), except that it takes multiple arguments.
 */
int
stdlog_log_b(stdlog_channel_t ch, const int severity,
	char *__restrict__ const wrkbuf, const size_t buflen,
	const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	return stdlog_vlog_b(ch, severity, wrkbuf, buflen, fmt, ap);
}

