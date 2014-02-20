lib_LTLIBRARIES = liblogging-stdlog.la
liblogging_stdlog_la_CPPFLAGS =
liblogging_stdlog_la_LIBADD =  $(SOL_LIBS)

liblogging_stdlog_la_SOURCES = \
	stdlog.c \
	uxsock.c \
	file.c \
	formatter.c \
	timeutils.c

pkginclude_HEADERS = \
	stdlog.h

noinst_HEADERS =
noinst_PROGRAMS = tester

tester_SOURCES = tester.c
tester_LDADD = liblogging-stdlog.la $(SOL_LIBS)

EXTRA_DIST = 

if ENABLE_JOURNAL
   liblogging_stdlog_la_LIBADD +=  $(LIBSYSTEMD_JOURNAL_LIBS)
   liblogging_stdlog_la_SOURCES += jrnldrvr.c
   # tester_LDADD +=  $(LIBSYSTEMD_JOURNAL_LIBS)
endif
