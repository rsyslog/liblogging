LIBLOGGING_SRC = beepchannel.c beepframe.c beepmessage.c beepprofile.c \
	beepsession.c sockets.c srAPI.c namevaluetree.c stringbuf.c \
	beepsession-lstn.c lstnprof-3195raw.c beeplisten.c srAPI-lstn.c \
	syslogmessage.c
LIBLOGGING_OBJ = $(LIBLOGGING_SRC:.c=.o)
LIBLOGGING_H = beepchannel.h beepframe.h  beepmessage.h beepprofile.h  beepsession.h \
	config.h liblogging.h namevaluetree.h sockets.h srAPI.h \
	namevaluetree.h stringbuf.h beeplisten.h lstnprof-3195raw.h \
	syslogmessage.h

TESTSRVR_SRC = testsrvr.c
TESTSRVR_OBJ = $(TESTSRVR_SRC:.c=.o)

TESTDRVR_SRC = testdrvr.c
TESTDRVR_OBJ = $(TESTDRVR_SRC:.c=.o)

# Uncomment to force gcc
#CC=gcc

# Uncomment for Solaris
#EXTRA_LIBS = -lsocket -lnsl

# Uncomment for debugging
DEBUG = -g

CFLAGS += $(DEBUG)

all: liblogging.a testdrvr testsrvr

liblogging.a: $(LIBLOGGING_OBJ) $(LIBLOGGING_H)
	$(AR) cru $@ $(LIBLOGGING_OBJ) 


testsrvr: liblogging.a $(TESTSRVR_OBJ)
	$(CC) -o $@ $(TESTSRVR_OBJ) -L. -llogging $(EXTRA_LIBS)

testdrvr: liblogging.a $(TESTDRVR_OBJ)
	$(CC) -o $@ $(TESTDRVR_OBJ) -L. -llogging $(EXTRA_LIBS)

clean:
	$(RM) liblogging.a
	$(RM) testdrvr
	$(RM) testsrvr
	$(RM) *.o
	$(RM) core

# Special dependency.
socket.o: sockets.c socketsUnix.c sockets.h
