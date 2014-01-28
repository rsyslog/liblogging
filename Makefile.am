SUBDIRS = 

pkgconfigdir = $(libdir)/pkgconfig

if ENABLE_STDLOG
pkgconfig_DATA = stdlog/libstdlog.pc
SUBDIRS += stdlog
endif

if ENABLE_RFC3195
pkgconfig_DATA = rfc3195/liblogging.pc
SUBDIRS += rfc3195/doc rfc3195/src
endif

