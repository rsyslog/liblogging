#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(liblogging, 1.0, rgerhards@hq.adiscon.com)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/beepchannel.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

AC_PROG_LIBTOOL

# Checks for libraries.
# FIXME: Replace `main' with a function in `-llogging':
#AC_CHECK_LIB([logging], [main])
# FIXME: Replace `main' with a function in `-lnsl':
#AC_CHECK_LIB([nsl], [main])
# FIXME: Replace `main' with a function in `-lsocket':
#AC_CHECK_LIB([socket], [main])

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

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 doc/html/Makefile
                 src/Makefile])
AC_OUTPUT
