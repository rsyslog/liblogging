VPATH = ..

CC = gcc

# This is really only needed when using gcc ... as cc links them in by
# default.  However, they don't hurt anything.
EXTRA_LIBS = -lsocket -lnsl

# Tell config.h we are compiling on solaris.
CFLAGS+=-DSROS_Solaris_

include ../makefile
