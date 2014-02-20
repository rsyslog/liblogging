/* The stdlog unix socket driver. Syslog protocol is spoken on
 * all unix sockets.
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
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "stdlog-intern.h"
#include "stdlog.h"

#define _PATH_LOG "/dev/log" /* default syslog socket on Linux */

static int
build_syslog_frame(stdlog_channel_t ch,
	const int severity,
	char *__restrict__ const frame,
	const size_t lenframe,
	const char *__restrict__ const msgbuf,
	size_t lenmsg)
{
	int i = 0;
	struct tm tm;
	int64_t pri;
	const time_t t = time(NULL);

	pri = (ch->facility << 3) | (severity & 0x07);
	__stdlog_timesub(&t, 0, &tm);
	frame[i++] = '<';
	__stdlog_fmt_print_int(frame, lenframe-i, &i, pri); 
	frame[i++] = '>';
	i += __stdlog_formatTimestamp3164(&tm, frame+i);
	frame[i++] = ' ';
	__stdlog_fmt_print_str(frame, lenframe-i, &i, ch->ident);
	frame[i++] = ':';
	frame[i++] = ' ';

	/* check overflow and truncate, if necessary */
	if(i + lenmsg > lenframe)
		lenmsg = lenframe - i;
	__stdlog_sigsafe_memcpy(frame+i, msgbuf, lenmsg);
	i += lenmsg;
	return i;
}

static void
uxs_init(stdlog_channel_t ch)
{
	ch->d.uxs.sock = -1;
}

static void
uxs_open(stdlog_channel_t ch)
{
	if (ch->d.uxs.sock == -1) {
		if((ch->d.uxs.sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
			return;
		memset(&ch->d.uxs.addr, 0, sizeof(ch->d.uxs.addr));
		ch->d.uxs.addr.sun_family = AF_UNIX;
		strncpy(ch->d.uxs.addr.sun_path, _PATH_LOG,
			sizeof(ch->d.uxs.addr.sun_path));
	}
}

static void
uxs_close(stdlog_channel_t ch)
{
	if (ch->d.uxs.sock >= 0) {
		close(ch->d.uxs.sock);
		ch->d.uxs.sock = -1;
	}
}


static void
uxs_log(stdlog_channel_t ch, int severity, char *__restrict__ msgbuf, const size_t lenmsg)
{
	ssize_t lsent;
	char frame[__STDLOG_MSGBUF_SIZE+256];
	size_t lenframe;
	printf("syslog got: '%s'\n", msgbuf);

	if(ch->d.uxs.sock < 0)
		uxs_open(ch);
	if(ch->d.uxs.sock < 0)
		return;
	lenframe = build_syslog_frame(ch, severity, frame, sizeof(frame), msgbuf, lenmsg);
printf("syslog frame: '%s'\n", frame);
	// TODO: error handling!!!
	lsent = sendto(ch->d.uxs.sock, frame, lenframe, 0,
		(struct sockaddr*) &ch->d.uxs.addr, sizeof(ch->d.uxs.addr));
	printf("sock: %d, lsent: %d\n", ch->d.uxs.sock, lsent);
	perror("send");
}

void
__stdlog_set_uxs_drvr(stdlog_channel_t ch)
{
	ch->drvr.init = uxs_init;
	ch->drvr.open = uxs_open;
	ch->drvr.close = uxs_close;
	ch->drvr.log = uxs_log;
}
