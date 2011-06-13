AC_PREREQ([2.61])
AC_INIT([vbig], [0.1], [rjk@greenend.org.uk])
AC_CONFIG_AUX_DIR([config.aux])
AM_INIT_AUTOMAKE([1.10])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AC_CONFIG_SRCDIR([vbig.cc])
AM_CONFIG_HEADER([config.h])
AC_LANG([C++])
AC_PROG_CXX
AC_PROG_RANLIB
AC_SET_MAKE
AC_DEFINE([_GNU_SOURCE], [1], [use GNU extensions])
if test "x$GXX" = xyes; then
  CXX="$CXX -Wall -W -Werror -Wpointer-arith -Wwrite-strings"
fi
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
