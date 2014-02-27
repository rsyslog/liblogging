SUBDIRS = 
pkgconfig_DATA =

pkgconfigdir = $(libdir)/pkgconfig

if ENABLE_STDLOG
pkgconfig_DATA += stdlog/liblogging-stdlog.pc
SUBDIRS += stdlog
endif

if ENABLE_RFC3195
pkgconfig_DATA += rfc3195/liblogging-rfc3195.pc
SUBDIRS += rfc3195/doc rfc3195/src
endif

ACLOCAL_AMFLAGS=-I m4
