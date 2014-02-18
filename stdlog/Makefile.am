lib_LTLIBRARIES = liblogging-stdlog.la
liblogging_la_CPPFLAGS =

liblogging_stdlog_la_SOURCES = \
	stdlog.c \
	uxsock.c \
	timeutils.c

if ENABLE_JOURNAL
   liblogging_la_CPPFLAGS = $(LIBSYSTEMD_JOURNAL_CFLAGS)
   liblogging_stdlog_la_SOURCES += jrnldrvr.c
endif

pkginclude_HEADERS = \
	stdlog.h

noinst_HEADERS =
noinst_PROGRAMS = tester

tester_SOURCES = tester.c
tester_LDADD = liblogging-stdlog.la

EXTRA_DIST = 
