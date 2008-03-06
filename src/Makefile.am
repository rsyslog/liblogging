lib_LTLIBRARIES = liblogging.la

liblogging_la_SOURCES = \
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
	srUtils.c
	stringbuf.c \
	syslogmessage.c

include_HEADERS = \
	liblogging.h \
	srAPI.h \
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
testsrvr_LDADD = liblogging.la

testdrvr_SOURCES = testdrvr.c
testdrvr_LDADD = liblogging.la
