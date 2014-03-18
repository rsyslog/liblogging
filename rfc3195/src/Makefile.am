lib_LTLIBRARIES = liblogging-rfc3195.la

liblogging_rfc3195_la_CFLAGS = ${AM_CFLAGS}
liblogging_rfc3195_la_LDFLAGS = \
	-version-info 0:0:0 \
	-export-symbols-regex '(^(srAPI|srSLMG).*)'
liblogging_rfc3195_la_SOURCES = \
	beepchannel.c \
	beepframe.c \
	beeplisten.c \
	beepmessage.c \
	beepprofile.c \
	beepsession.c \
	beepsession-lstn.c \
	clntprof-3195cooked.c \
	clntprof-3195raw.c \
	lstnprof-3195raw.c \
	lstnprof-3195cooked.c \
	namevaluetree.c \
	oscalls.c \
	sockets.c \
	srAPI.c \
	srAPI-lstn.c \
	srUtils.c \
	stringbuf.c \
	syslogmessage.c

os_sources = \
	oscallsUnix.c \
	oscallsWin32.c \
	socketsUnix.c \
	socketsWin32.c


pkginclude_HEADERS = \
	liblogging.h \
	srAPI.h \
	settings.h \
	syslogmessage.h

noinst_HEADERS = \
	beepchannel.h \
	beepframe.h \
	beeplisten.h \
	beepmessage.h \
	beepprofile.h \
	beepsession.h \
	clntprof-3195cooked.h \
	clntprof-3195raw.h \
	lstnprof-3195cooked.h \
	lstnprof-3195raw.h \
	namevaluetree.h \
	oscalls.h \
	sockets.h \
	srUtils.h \
	stringbuf.h

noinst_PROGRAMS = testsrvr testdrvr

testsrvr_SOURCES = testsrvr.c
testsrvr_la_CFLAGS = ${AM_CFLAGS}
testsrvr_LDADD = liblogging-rfc3195.la

testdrvr_SOURCES = testdrvr.c
testdrvr_la_CFLAGS = ${AM_CFLAGS}
testdrvr_LDADD = liblogging-rfc3195.la

EXTRA_DIST = $(os_sources)
