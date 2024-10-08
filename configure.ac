#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([liblogging], [1.0.7], [rgerhards@adiscon.com])
AM_INIT_AUTOMAKE
# AIXPORT START: Detect the underlying OS
unamestr=$(uname)
AM_CONDITIONAL([AIX], [test x$unamestr = xAIX])
# AIXPORT : Set the required variables for AIX config script
if test "$unamestr" = "AIX"; then
	export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/lib/pkgconfig"
	CC=xlc
	CFLAGS="-qcpluscmt"
	CPPFLAGS="-D_AIX -D_BSD=43 -D_THREAD_SAFE"
	LIBS="-lbsd "
	LDFLAGS="-brtl -bexpall"
	AC_PREFIX_DEFAULT(/usr)
	export ac_cv_func_malloc_0_nonnull=yes
fi
# AIXPORT End
AC_CONFIG_SRCDIR([rfc3195/src/beepchannel.c])
AC_CONFIG_HEADER([config.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
PKG_PROG_PKG_CONFIG
AC_PROG_CC
# AIXPORT START: enable dlopen
if test "$unamestr" = "AIX"; then
	AC_LIBTOOL_DLOPEN
fi
# AIXPORT end
AC_PROG_LIBTOOL
AM_PROG_CC_C_O
if test "$GCC" = "yes"
   then AM_CFLAGS="-W -Wall -Wformat-security -Wshadow -Wcast-align -Wpointer-arith -Wmissing-format-attribute -g"
fi

case "${host}" in
  *-*-linux*)
    os_type="linux"
  ;;
  *-*-*darwin*|*-*-freebsd*|*-*-netbsd*|*-*-openbsd*)
    AC_DEFINE([OS_BSD], [1], [Indicator for a BSD OS])
    os_type="bsd"
  ;;
  *-*-kfreebsd*)
    # kernel is FreeBSD, but userspace is glibc - i.e. like linux
    # do not DEFINE OS_BSD
    os_type="bsd"
  ;;
  *-*-solaris*)
    os_type="solaris"
    AC_DEFINE([OS_SOLARIS], [1], [Indicator for a Solaris OS])
    SOL_LIBS="-lsocket -lnsl"
    AC_SUBST(SOL_LIBS)
  ;;
esac

save_LIBS=$LIBS
LIBS=
#AC_SEARCH_LIBS(clock_gettime, rt)
rt_libs=$LIBS
LIBS=$save_LIBS

AC_SUBST(rt_libs)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([gethostbyname gethostname gettimeofday inet_ntoa memset select socket])


# rfc3195 component
AC_ARG_ENABLE(rfc3195,
        [AS_HELP_STRING([--enable-rfc3195],[Enable rfc3195 support @<:@default=no@:>@])],
        [case "${enableval}" in
         yes) enable_rfc3195="yes" ;;
          no) enable_rfc3195="no" ;;
           *) AC_MSG_ERROR(bad value ${enableval} for --enable-rfc3195) ;;
         esac],
        [enable_rfc3195="no"]
)
AM_CONDITIONAL(ENABLE_RFC3195, test x$enable_rfc3195 = xyes)
if test "$enable_rfc3195" = "yes"; then
        AC_DEFINE(LIBLOGGING_STDLOG, 1, [rfc3195 support is integrated.])
fi


## stdlog component
AC_ARG_ENABLE(stdlog,
        [AS_HELP_STRING([--enable-stdlog],[Enable stdlog support @<:@default=yes@:>@])],
        [case "${enableval}" in
         yes) enable_stdlog="yes" ;;
          no) enable_stdlog="no" ;;
           *) AC_MSG_ERROR(bad value ${enableval} for --enable-stdlog) ;;
         esac],
        [enable_stdlog="yes"]
)
AM_CONDITIONAL(ENABLE_STDLOG, test x$enable_stdlog = xyes)
if test "$enable_stdlog" = "yes"; then
        AC_DEFINE(LIBLOGGING_STDLOG, 1, [stdlog support is integrated.])
fi

# journal support to be added
# if so, this enables the journal driver in liblogging-stdlog
AC_ARG_ENABLE(journal,
        [AS_HELP_STRING([--enable-journal],[systemd journal support (yes/no/auto) @<:@default=auto@:>@])],
        [case "${enableval}" in
         auto) enable_journal="auto" ;;
         yes) enable_journal="yes" ;;
          no) enable_journal="no" ;;
           *) AC_MSG_ERROR(bad value ${enableval} for --enable-journal) ;;
         esac],
        [enable_journal="auto"]
)
if test "x$enable_journal" = "xauto"; then
    PKG_CHECK_MODULES([LIBSYSTEMD_JOURNAL], [libsystemd >= 209] ,, [
	    PKG_CHECK_MODULES([LIBSYSTEMD_JOURNAL], [libsystemd-journal >= 197] ,,  [
	        AC_MSG_RESULT([journal libraries not found, assuming this system is not using systemd journal. Disabling the journal driver.])
	    	enable_journal="no"
	    ])
	])
fi
if test "x$enable_journal" = "xyes"; then
	PKG_CHECK_MODULES([LIBSYSTEMD_JOURNAL], [libsystemd >= 209] ,, [
	    PKG_CHECK_MODULES([LIBSYSTEMD_JOURNAL], [libsystemd-journal >= 197])
	])
fi
AM_CONDITIONAL(ENABLE_JOURNAL, test x$enable_journal = xyes)
if test "$enable_journal" = "yes"; then
        AC_DEFINE(ENABLE_JOURNAL, 1, [journal support is integrated.])
fi

# should man pages be provided?
AC_ARG_ENABLE(man-pages,
        [AS_HELP_STRING([--enable-man-pages],[man-pages support @<:@default=yes@:>@])],
        [case "${enableval}" in
         yes) enable_man_pages="yes" ;;
          no) enable_man_pages="no" ;;
           *) AC_MSG_ERROR(bad value ${enableval} for --enable-man-pages) ;;
         esac],
        [enable_man_pages="yes"]
)
if test "x$enable_man_pages" = "xyes"; then
	AC_CHECKING([if all man pages already exist])
	have_to_generate_man_pages="no"
	if test ! -r "stdlog/stdlog.3" || test ! -r "stdlog/stdlogctl.1"; then
		have_to_generate_man_pages="yes"
	fi
	if test "x$have_to_generate_man_pages" = "xyes"; then
	    AC_MSG_RESULT([Some man pages are missing. We need rst2man to generate the missing man pages from source... Alternatively, use --disable-man-pages to build without them.])
	else
	    AC_MSG_RESULT([All man pages found. We don't need rst2man!])
	fi

	if test "x$have_to_generate_man_pages" = "xyes"; then
 	    # We need rst2man to generate our man pages
	    AC_CHECK_PROGS([RST2MAN], [rst2man rst2man.py], [])
	    if test -z "$RST2MAN"; then
	        AC_MSG_ERROR([rst2man is required to build man pages. You can use the release tarball with pregenerated man pages to avoid this depedency. Alternatively, use --disable-man-pages to build without them.])
	    fi
	fi
fi
AM_CONDITIONAL(ENABLE_MAN_PAGES, test x$enable_man_pages = xyes)

AC_CONFIG_FILES([Makefile \
                 rfc3195/liblogging-rfc3195.pc \
                 rfc3195/doc/Makefile \
                 rfc3195/doc/html/Makefile \
                 rfc3195/src/Makefile \
                 stdlog/liblogging-stdlog.pc \
		 stdlog/Makefile])
AC_OUTPUT

echo "********************************************************"
echo "liblogging will be compiled with the following settings:"
echo
echo "stdlog support enabled:               $enable_stdlog"
echo "systemd journal support:              $enable_journal"
echo "rfc3195 support enabled:              $enable_rfc3195"
echo "enable man pages:                     $enable_man_pages"
if test "x$enable_man_pages" = "xyes"; then
echo "have to generate man pages:           $have_to_generate_man_pages"
fi
echo "********************************************************"
