/* The stdlog main header file
 * Copyright (C) 2014 by Rainer Gerhards and Adiscon GmbH
 * Released under the ASL 2.0
 */

#ifndef LIBSTDLOG_H_INCLUDED
#define LIBSTDLOG_H_INCLUDED

int
stdlog_log(int channel, const int severity, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

#endif /* multi-include protection */
