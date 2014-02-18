/* The stdlog formatter helper library.
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

/* do not return ptr to dest as an optimization -- should barely be needed! */
void
__stdlog_sigsafe_memcpy(void *__restrict__ dest, const void *__restrict__ src, size_t n)
{
	char *d = (char*) dest, *s = (char*) src;
	while(n--)
		*d++ = *s++;
}

static size_t
__stdlog_sigsafe_strlen(const char *__restrict__ s)
{
	size_t len = 0;
	while(*s++)
		len++;
	return len;
}

void
__stdlog_fmt_print_int (char *__restrict__ const buf, const size_t lenbuf,
	int *idx, int64_t nbr)
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

void
__stdlog_fmt_print_str (char *__restrict__ const buf, const size_t lenbuf,
	int *__restrict__ const idx, const char *const str)
{
	size_t lenstr = __stdlog_sigsafe_strlen(str);
	__stdlog_sigsafe_memcpy(buf+(*idx), str, lenstr);
	// TODO: check limits!
	*idx += lenstr;
}

size_t
__stdlog_fmt_printf(char *buf, size_t lenbuf, const char *fmt, va_list ap)
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
				__stdlog_fmt_print_str(buf, lenbuf, &i, s);
				break;
			case 'd':
				d = va_arg(ap, int);
				__stdlog_fmt_print_int(buf, lenbuf, &i, (int64_t) d); 
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
