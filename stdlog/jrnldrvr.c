/* The stdlog systemd journal driver
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
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <systemd/sd-journal.h>
#include "stdlog-intern.h"
#include "stdlog.h"

/* dummies just needed for driver interface */
static void jrnl_init(stdlog_channel_t __attribute__((unused)) ch) { }
static void jrnl_open(stdlog_channel_t __attribute__((unused)) ch) { }
static void jrnl_close(stdlog_channel_t __attribute__((unused)) ch) { }

static int
jrnl_log(stdlog_channel_t ch, const int severity,
	const char *fmt, va_list ap,
	char *__restrict__ const wrkbuf, const size_t buflen)
{
	int r;
	ch->f_vsnprintf(wrkbuf, buflen, fmt, ap);
	r = sd_journal_send("MESSAGE=%s", wrkbuf,
                "PRIORITY=%d", severity,
                NULL);
	if(r) errno = -r;
	return r;
}

void
__stdlog_set_jrnl_drvr(stdlog_channel_t ch)
{
	ch->drvr.init = jrnl_init;
	ch->drvr.open = jrnl_open;
	ch->drvr.close = jrnl_close;
	ch->drvr.log = jrnl_log;
}
