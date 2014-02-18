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
static char *dflt_chanspec = NULL;


/* can be called before any other library call. If so, initializes
 * some library strucutres.
 * NOTE: this API may change in the final version
 * returns 0 on success, -1 on error with errno set
 */
int
stdlog_init(void)
{
	char *chanspec;

printf("ini init, dch %p\n", dflt_channel);
	if (dflt_channel != NULL) {
		errno = EINVAL;
		return -1;
	}

printf("in2 init\n");
	chanspec = getenv("LIBLOGGING_STDLOG_DFLT_LOG_DESTINATION");
	if (chanspec == NULL)
		chanspec = "syslog:";
	if ((dflt_chanspec = strdup(chanspec)) == NULL)
		return -1;

	if((dflt_channel = 
	      stdlog_open("TEST", 0, 3, NULL)) == NULL)
		return -1;
printf("out init\n");

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

/* interprets a driver chanspec and sets the channel accordingly
 */
static int
__stdlog_set_driver(stdlog_channel_t ch, const char *__restrict__ chanspec)
{
	ch->driver = 0;
	if (chanspec == NULL)
		chanspec = dflt_chanspec;
printf("chanspec: '%s'\n", chanspec);

	if((ch->spec = strdup(chanspec)) == NULL) {
		errno = ENOMEM;
		return -1;
	}

	if (!strcmp(chanspec, "journal:"))
		ch->driver = 1;
	printf("driver %d set\n", ch->driver);
	return 0;
}

stdlog_channel_t
stdlog_open(const char *ident, int option, int facility, const char *chanspec)
{
	stdlog_channel_t ch;

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
	ch->options = option;
	ch->facility = facility;

	if(__stdlog_set_driver(ch, chanspec) != 0) {
		int errnosv = errno;
		free((char*)ch->ident);
		free((char*)ch->spec);
		free(ch);
		ch = NULL;
		errno = errnosv;
		goto done;
	}

	ch->d.uxs.sock = -1;
done:
	return ch;
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

static size_t
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
	return i;
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
	i += my_printf(msg+i, lenmsg-i, fmt, ap);
	ch->lenmsg = i;
}

/* TODO: move to driver layer */
static void
format_jrnl(stdlog_channel_t ch,
	const int severity,
	const char *__restrict__ const fmt,
	va_list ap)
{

	ch->lenmsg = my_printf(ch->msgbuf, sizeof(ch->msgbuf), fmt, ap);
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
		if (dflt_channel == NULL)
			if((r = stdlog_init()) != 0)
				goto done;
		ch = dflt_channel;
	}
	va_start(ap, fmt);
	if(ch->driver == 1) {
		format_jrnl(ch, severity, fmt, ap);
		__stdlog_jrnl_log(ch, severity);
	} else {
		format_syslog(ch, severity, fmt, ap);
		__stdlog_uxs_log(ch);
	}

done:	return r;
}
