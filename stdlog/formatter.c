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

#if 0 /* currently not used, but do not want to remove */
static size_t
__stdlog_sigsafe_strlen(const char *__restrict__ s)
{
	size_t len = 0;
	while(*s++)
		len++;
	return len;
}
#endif

static int
__stdlog_isdigit(const char c)
{
	return (c >= '0' && c <='9') ? 1 : 0;
}
/* hexbase must be 'a' for lower case hex string or 'A' for
 * upper case. Anything else is invalid.
 */
void
__stdlog_fmt_print_uint_hex (char *__restrict__ const buf, const size_t lenbuf,
	int *idx, uint64_t nbr, const char hexbase)
{
	size_t i;
	int j;
	int hexdigit;
	char numbuf[17];

	if (nbr == 0) {
		buf[(*idx)++] = '0';
		goto done;
	}
	j = 0;
	while(nbr != 0) {
		hexdigit = nbr % 16;
		numbuf[j++] = (hexdigit < 10) ? hexdigit + '0'
					      : hexdigit - 10 + hexbase;
		nbr /= 16;
	}
	for(i = *idx, --j; i < lenbuf && j >= 0 ; ++i, --j)
		buf[i] = numbuf[j];

	*idx = i;

done:	return;
}

void
__stdlog_fmt_print_uint (char *__restrict__ const buf, const size_t lenbuf,
	int *idx, uint64_t nbr)
{
	size_t i;
	int j;
	char numbuf[21];

	if (nbr == 0) {
		buf[(*idx)++] = '0';
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
__stdlog_fmt_print_int (char *__restrict__ const buf, const size_t lenbuf,
	int *idx, int64_t nbr)
{
	if (nbr == 0) {
		buf[(*idx)++] = '0';
		goto done;
	}
	if (nbr < 0) {
		buf[(*idx)++] = '-';
		nbr *= -1;
	}
	__stdlog_fmt_print_uint(buf, lenbuf, idx, (uint64_t)nbr);
done:	return;
}

/**
 * Format a double number as %.2f.
 * Base idea taken from x.org source ./os/utils.c
 */
void
__stdlog_fmt_print_double (char *__restrict__ const buf, const size_t lenbuf,
	int *idx, double dbl)
{
	uint64_t frac;

	frac = (dbl > 0 ? dbl : -dbl) * 100.0 + 0.5;
	frac %= 100;

	/* write decimal part to string */
	__stdlog_fmt_print_int(buf, lenbuf, idx, (int64_t)dbl);

	/* append fractional part, but only if there is space */
	if (*idx < (int)lenbuf) {
		buf[(*idx)++] = '.';
		/* TODO: change _print_int to support min size! */
		if (frac < 10) { /* work.around for missing _print_int feature! */
			if(*idx < (int)lenbuf)
				buf[(*idx)++] = '0';
		}
	__stdlog_fmt_print_int(buf, lenbuf, idx, frac);
	}
}


void
__stdlog_fmt_print_str (char *__restrict__ const buf, const size_t lenbuf,
	int *__restrict__ const idx, const char *const str)
{
	int i = *idx;
	int j = 0;

	while(i < (int) lenbuf && str[j])
		buf[i++] = str[j++];
	*idx = i;
}

/* This is a big monolythic function to save us hassle with the
 * va_list macros (and do not loose performance solving that
 * hassle...).
 */
int
__stdlog_sigsafe_printf(char *buf, size_t lenbuf, const char *fmt, va_list ap)
{
	char *s;
	int64_t d;
	uint64_t u;
	int fwidth;
	int precision;
	int i = 0;
	double dbl;
	enum {LMOD_LONG, LMOD_LONG_LONG,
	      LMOD_SIZE_T, LMOD_SHORT, LMOD_CHAR} length_modifier;

	--lenbuf; /* reserve for terminal \0 */
	while(*fmt && i < (int) lenbuf) {
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
			/* TODO: implement others -- and think if we want these
			 * in log messages in the first place...
			 */
			default:
				buf[i++] = *fmt;
				break;
			}
			break;
		case '%':
			if(*++fmt == '\0') goto done;

			/* width */
			/* currently only read, but not processed.
			 * TODO: actually use fwidth and precision?
			 */
			fwidth = precision = -1;
			if(__stdlog_isdigit(*fmt)) {
				fwidth = 0;
				for(; __stdlog_isdigit(*fmt) ; ++fmt)
					fwidth = fwidth * 10 + *fmt - '0';
				if(*fmt == '.') {
					precision = 0;
					for(++fmt ; __stdlog_isdigit(*fmt) ; ++fmt)
						precision = precision * 10 + *fmt - '0';
				}
			}
		
			/* length modifiers */
			length_modifier = 0;
			switch(*fmt) {
			case '\0':
				goto done;
			case 'l':
				++fmt;
				if (*fmt == 'l') {
					++fmt;
					length_modifier = LMOD_LONG_LONG;
				} else {
					length_modifier = LMOD_LONG;
				}
				break;
			case 'h':
				++fmt;
				if (*fmt == 'h') {
					++fmt;
					length_modifier = LMOD_CHAR;
				} else {
					length_modifier = LMOD_SHORT;
				}
				break;
			case 'z':
				++fmt;
				length_modifier = LMOD_SIZE_T;
				break;
			default:break; /* no modifier, nothing to do */
			}

			/* conversions */
			switch(*fmt) {
			case '\0':
				goto done;
			case 's':
				s = va_arg(ap, char *);
				__stdlog_fmt_print_str(buf, lenbuf, &i, s);
				break;
			case 'i':
			case 'd':
				if (length_modifier == LMOD_LONG_LONG)
					d = va_arg(ap, long long);
				else if (length_modifier == LMOD_LONG)
					d = va_arg(ap, long);
				else if (length_modifier == LMOD_SIZE_T)
					d = va_arg(ap, ssize_t);
				else
					d = va_arg(ap, int);
				__stdlog_fmt_print_int(buf, lenbuf, &i, d); 
				break;
			case 'u':
			case 'x':
			case 'X':
				if (length_modifier == LMOD_LONG_LONG)
					u = va_arg(ap, unsigned long long);
				else if (length_modifier == LMOD_LONG)
					u = va_arg(ap, unsigned long);
				else if (length_modifier == LMOD_SIZE_T)
					u = va_arg(ap, size_t);
				else
					u = va_arg(ap, unsigned);
				if(*fmt == 'u')
					__stdlog_fmt_print_uint(buf, lenbuf, &i, u); 
				else
					__stdlog_fmt_print_uint_hex(buf, lenbuf, &i,
						u, *fmt == 'x'? 'a' : 'A'); 
				break;
			case 'p':
				u = (uintptr_t) va_arg(ap, void *);
				if (u == 0) {
					__stdlog_fmt_print_str(buf, lenbuf, &i, "(null)");
				} else {
					__stdlog_fmt_print_str(buf, lenbuf, &i, "0x");
					__stdlog_fmt_print_uint_hex(buf, lenbuf, &i, u, 'a'); 
				}
				break;
			case 'f':
				dbl = va_arg(ap, double);
				__stdlog_fmt_print_double(buf, lenbuf, &i, dbl); 
				break;
			case 'c':
				buf[i++] = (char) va_arg(ap, int);
				break;
			case '%':
				buf[i++] = '%';
				break;
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

/* wrapper for standard vsnprintf() as we need to return it the number of
 * bytes actually written (minus the NUL char) - even in overlow case!
 */
int
__stdlog_wrapper_vsnprintf(char *buf, size_t lenbuf, const char *fmt, va_list ap)
{
	int len;

	len = vsnprintf(buf, lenbuf, fmt, ap);
	if (len >= (int)lenbuf)
		len = (int) lenbuf;
	return len;
}
