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
EXTRA_DIST = stdlog-intern.h \
	stdlog.rst \
	stdlogctl.rst

pkginclude_HEADERS = \
	stdlog.h

noinst_HEADERS =
noinst_PROGRAMS = tester
bin_PROGRAMS =

tester_SOURCES = tester.c
tester_LDADD = liblogging-stdlog.la $(SOL_LIBS)

bin_PROGRAMS += stdlogctl
stdlogctl_SOURCES = stdlogctl.c
stdlogctl_CPPFLAGS =  
stdlogctl_LDADD = liblogging-stdlog.la $(SOL_LIBS)

man1_MANS = stdlogctl.1
CLEANFILES += stdlogctl.1
EXTRA_DIST += stdlogctl.1

man3_MANS = stdlog.3
CLEANFILES += stdlog.3
EXTRA_DIST += stdlog.3

stdlogctl.1: stdlogctl.rst
	$(AM_V_GEN) $(RST2MAN) $< $@

stdlog.3: stdlog.rst
	$(AM_V_GEN) $(RST2MAN) $< $@

if ENABLE_JOURNAL
   liblogging_stdlog_la_LIBADD +=  $(LIBSYSTEMD_JOURNAL_LIBS)
   liblogging_stdlog_la_SOURCES += jrnldrvr.c
   # tester_LDADD +=  $(LIBSYSTEMD_JOURNAL_LIBS)
endif
