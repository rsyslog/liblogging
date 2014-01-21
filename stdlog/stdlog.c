/* The stdlog main file
 * Copyright (C) 2014 by Rainer Gerhards and Adiscon GmbH
 * This is currently a small chim, which enables us to inject the full
 * functionality at a later point in time.
 * Released under the ASL 2.0
 */
#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include "libstdlog.h"

/* returns 0 on success or a standard (negative) error code */
int
stdlog_log(stdlog_channel_t __attribute((unused)) channel,
	const int severity, const char *fmt, ...)
{
	char msg[4096];
	va_list ap;
	int r;

	va_start(ap, fmt);
	r = vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end (ap);
	if(r < 0) goto done;
		r = 0;

	syslog(severity, "%s", msg);

done:	return r;
}
