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
build_file_line(stdlog_channel_t ch, const int severity)
{
	char *f = ch->d.file.lnbuf;
	size_t lenline = sizeof(ch->d.file.lnbuf);
	int i = 0;
	struct tm tm;
	const time_t t = time(NULL);

	__stdlog_timesub(&t, 0, &tm);
	i += __stdlog_formatTimestamp3164(&tm, f+i);
	f[i++] = ' ';
	__stdlog_fmt_print_str(f, lenline-i, &i, ch->ident);
	f[i++] = ':';
	f[i++] = ' ';
	__stdlog_sigsafe_memcpy(f+i, ch->msgbuf, ch->lenmsg);
	i += ch->lenmsg;
	f[i++] = '\n';
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
		if((ch->d.file.fd = open(ch->d.file.name, O_WRONLY|O_CREAT, 0660)) < 0)
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


static void
file_log(stdlog_channel_t ch, int severity)
{
	size_t lenline;
	ssize_t lwritten;
	printf("syslog got: '%s'\n", ch->msgbuf);

	if(ch->d.file.fd < 0)
		file_open(ch);
	if(ch->d.file.fd < 0)
		return;
	lenline = build_file_line(ch, severity);
printf("file line: '%s'\n", ch->d.file.lnbuf);
	// TODO: error handling!!!
	lwritten = write(ch->d.file.fd, ch->d.file.lnbuf, lenline);
	printf("fd: %d, lwritten: %d\n", ch->d.file.fd, lwritten);
}

void
__stdlog_set_file_drvr(stdlog_channel_t ch)
{
	ch->drvr.init = file_init;
	ch->drvr.open = file_open;
	ch->drvr.close = file_close;
	ch->drvr.log = file_log;
}
