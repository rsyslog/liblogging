/* The stdlog file driver.
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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "stdlog-intern.h"
#include "stdlog.h"

static int
build_file_line(stdlog_channel_t ch,
	char *__restrict__ const linebuf,
	const size_t lenline,
	const char *fmt,
	va_list ap)
{
	int i = 0;
	struct tm tm;
	const time_t t = time(NULL);

	__stdlog_timesub(&t, 0, &tm);
	i += __stdlog_formatTimestamp3164(&tm, linebuf+i);
	__STDLOG_STRBUILD_ADD_CHAR(linebuf, lenline, i, ' ');
	__stdlog_fmt_print_str(linebuf, lenline-i, &i, ch->ident);
	__STDLOG_STRBUILD_ADD_CHAR(linebuf, lenline, i, ':');
	__STDLOG_STRBUILD_ADD_CHAR(linebuf, lenline, i, ' ');
	/* note: we do not need to reserve space for '\0', as we
	 * will overwrite it with the '\n' below. We don't need 
	 * a string, just a buffer, so we don't need '\0'!
	 */
	i += ch->f_vsnprintf(linebuf+i, lenline-i, fmt, ap);
	linebuf[i++] = '\n'; /* space reserved -- see above */
	return i;
}

static void
file_init(stdlog_channel_t ch)
{
	ch->d.file.fd = -1;
	ch->d.file.name = strdup(ch->spec+5);
}

static void
file_open(stdlog_channel_t ch)
{
	if (ch->d.file.fd == -1) {
		if((ch->d.file.fd = open(ch->d.file.name, O_WRONLY|O_CREAT|O_APPEND, 0660)) < 0)
			return;
	}
}

static void
file_close(stdlog_channel_t ch)
{
	if (ch->d.file.fd >= 0) {
		close(ch->d.file.fd);
		ch->d.file.fd = -1;
	}
}


static int
file_log(stdlog_channel_t ch, int __attribute__((unused)) severity,
	const char *fmt, va_list ap,
	char *__restrict__ const wrkbuf, const size_t buflen)
{
	ssize_t lenWritten;
	size_t lenline;
	int r;

	if(ch->d.file.fd < 0)
		file_open(ch);
	if(ch->d.file.fd < 0) {
		r = -1;
		goto done;
	}
	lenline = build_file_line(ch, wrkbuf, buflen, fmt, ap);
	lenWritten = write(ch->d.file.fd, wrkbuf, lenline);
	if(lenWritten == -1) {
		r = -1;
	} else if(lenWritten != (ssize_t) lenline) {
		r = -1;
		errno = EAGAIN;
	} else {
		r = 0;
	}
done:	return r;
}

void
__stdlog_set_file_drvr(stdlog_channel_t ch)
{
	ch->drvr.init = file_init;
	ch->drvr.open = file_open;
	ch->drvr.close = file_close;
	ch->drvr.log = file_log;
}
