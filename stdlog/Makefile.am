lib_LTLIBRARIES = liblogging-stdlog.la

liblogging_stdlog_la_SOURCES = \
	stdlog.c \
	syslog.c

pkginclude_HEADERS = \
	stdlog.h

noinst_HEADERS =
noinst_PROGRAMS = tester

tester_SOURCES = tester.c
tester_LDADD = liblogging-stdlog.la

EXTRA_DIST = 
