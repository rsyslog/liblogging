EXTRA_DIST =
CLEANFILES =

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

man3_MANS = stdlog.3
CLEANFILES += stdlog.3
EXTRA_DIST += stdlog.3

stdlog.3: stdlog.rst
	$(AM_V_GEN) $(RST2MAN) $< $@

tester_SOURCES = tester.c
tester_LDADD = liblogging-stdlog.la $(SOL_LIBS)

if ENABLE_JOURNAL
   liblogging_stdlog_la_LIBADD +=  $(LIBSYSTEMD_JOURNAL_LIBS)
   liblogging_stdlog_la_SOURCES += jrnldrvr.c
   # tester_LDADD +=  $(LIBSYSTEMD_JOURNAL_LIBS)
endif
