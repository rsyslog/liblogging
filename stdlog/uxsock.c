/* The stdlog unix socket driver
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

#define _PATH_LOG "/dev/log" // default syslog socket on Linux

static void
__stdlog_uxs_open(stdlog_channel_t ch)
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
__stdlog_uxs_close(stdlog_channel_t ch)
{
	if (ch->d.uxs.sock >= 0) {
		close(ch->d.uxs.sock);
		ch->d.uxs.sock = -1;
	}
}


void
__stdlog_uxs_log(stdlog_channel_t ch, const char *__restrict__ const msg)
{
	const size_t lenmsg = strlen(msg);
	ssize_t lsent;
	printf("syslog got: '%s'\n", msg);

	if(ch->d.uxs.sock < 0)
		__stdlog_uxs_open(ch);
	if(ch->d.uxs.sock < 0)
		return;
	// TODO: error handling!!!
	lsent = sendto(ch->d.uxs.sock, msg, lenmsg, 0,
		(struct sockaddr*) &ch->d.uxs.addr, sizeof(ch->d.uxs.addr));
	printf("sock: %d, lsent: %d\n", ch->d.uxs.sock, lsent);
	perror("send");
}
