SUBDIRS = 

pkgconfigdir = $(libdir)/pkgconfig

if ENABLE_STDLOG
pkgconfig_DATA = stdlog/libstdlog.pc
SUBDIRS += stdlog
endif

if ENABLE_RFC3195
pkgconfig_DATA = liblogging.pc
SUBDIRS += doc src
endif

