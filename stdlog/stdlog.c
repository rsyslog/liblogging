/* The stdlog main file
 * This is currently a small shim, which enables us to inject the full
 * functionality at a later point in time.
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

stdlog_channel_t
stdlog_open(const char *ident, int option, int facility, const char *channelspec)
{
	stdlog_channel_t channel;

	if((channel = calloc(1, sizeof(struct stdlog_channel))) == NULL) {
		errno = ENOMEM;
		goto done;
	}
	if((channel->spec = strdup(channelspec)) == NULL) {
		free(channel);
		channel = NULL;
		errno = ENOMEM;
		goto done;
	}
	if((channel->ident = strdup(ident)) == NULL) {
		free((char*)channel->spec);
		free(channel);
		channel = NULL;
		errno = ENOMEM;
		goto done;
	}
	channel->options = option;
	channel->facility = facility;

	channel->d.uxs.sock = -1;
done:
	return channel;
}

void
stdlog_close(stdlog_channel_t channel)
{
	free(channel);
}


static void
print_int (char *__restrict__ const buf, const size_t lenbuf, int *idx, int64_t nbr)
{
	size_t i;
	int j;
	char numbuf[21];

	if (nbr == 0) {
		buf[*idx] = '0';
		(*idx)++;
		goto done;
	}
	j = 0;
	while(nbr != 0) {
		numbuf[j++] = nbr % 10 + '0';
		nbr /= 10;
	}
	for(i = *idx, --j; i < lenbuf && j >= 0 ; ++i, --j)
		buf[i] = numbuf[j];

	*idx = i;

done:	return;
}

static void
print_str (char *__restrict__ const buf, const size_t lenbuf, int *idx, const char *const str)
{
	size_t lenstr = strlen(str);
	memcpy(buf+(*idx), str, lenstr);
	// TODO: check limits!
	*idx += lenstr;
}

static void
my_printf(char *buf, size_t lenbuf, const char *fmt, va_list ap)
{
	char *s;
	int d;
	int i = 0;

	--lenbuf; /* reserve for terminal \0 */
	while(*fmt && i < (int) lenbuf) {
		printf("to process: '%c'\n", *fmt);
		switch(*fmt) {
		case '\\':
			if(*++fmt == '\0') goto done;
			switch(*fmt) {
			case 'n':
				buf[i++] = '\n';
				break;
			case 'r':
				buf[i++] = '\r';
				break;
			case 't':
				buf[i++] = '\t';
				break;
			case '\\':
				buf[i++] = '\\';
				break;
			// TODO: implement others
			default:
				buf[i++] = *fmt;
				break;
			}
			break;
		case '%':
			if(*++fmt == '\0') goto done;
			switch(*fmt) {
			case 's':
				s = va_arg(ap, char *);
				print_str(buf, lenbuf, &i, s);
				break;
			case 'd':
				d = va_arg(ap, int);
				print_int(buf, lenbuf, &i, (int64_t) d); 
				break;
			case 'c':
				buf[i++] = (char) va_arg(ap, int);
				break;
			// TODO: implement others
			default:
				buf[i++] = '?';
				break;
			}
			break;
		default:
			buf[i++] = *fmt;
			break;
		}
		++fmt;
	}
done:
	buf[i] = '\0'; /* we reserved space for this! */
	va_end(ap);
}


/* TODO: move to driver layer */
static void
format_syslog(stdlog_channel_t ch,
	const int severity,
	const char *__restrict__ const fmt,
	va_list ap)
{
	char *msg = ch->msgbuf;
	size_t lenmsg = sizeof(ch->msgbuf);
	int i = 0;
	struct tm tm;
	time_t t = time(NULL);

	__stdlog_timesub(&t, 0, &tm);
	msg[i++] = '<';
	print_int(msg, lenmsg-i, &i, (int64_t) severity); 
	msg[i++] = '>';
	i += __stdlog_formatTimestamp3164(&tm, msg+i);
	msg[i++] = ' ';
	print_str(msg, lenmsg-i, &i, ch->ident);
	msg[i++] = ':';
	msg[i++] = ' ';
	my_printf(msg+i, lenmsg-i, fmt, ap);
	ch->lenmsg = strlen(msg); // TODO: replace with index update <--> signal-safeness!!
}
	

/* Log a message to the specified channel. If channel is NULL,
 * use the default channel (which always exists).
 * Returns 0 on success or a standard (negative) error code.
 * Otherwise the semantics are equivalent to syslog().
 */
int
stdlog_log(stdlog_channel_t ch,
	const int severity, const char *fmt, ...)
{
	va_list ap;
	int r = 0;

	if(ch == NULL) {
		if (dflt_channel == NULL) {
			if((dflt_channel = 
			      stdlog_open("TEST", 0, 3, "...")) == NULL)
				goto done;
		}
		ch = dflt_channel;
	}
	va_start(ap, fmt);
	format_syslog(ch, severity, fmt, ap);
	printf("outputting: '%s'\n", ch->msgbuf);
	__stdlog_jrnl_log(ch, severity);

//	__stdlog_uxs_log(ch);

done:	return r;
}
