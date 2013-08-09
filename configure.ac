AC_PREREQ([2.61])
AC_INIT([vbig], [0.3], [rjk@greenend.org.uk])
m4_include([m4/ax_random_device.m4])
AC_CONFIG_AUX_DIR([config.aux])
AM_INIT_AUTOMAKE([foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AC_CONFIG_SRCDIR([vbig.cc])
AM_CONFIG_HEADER([config.h])
AC_CANONICAL_HOST
case "$host" in
*-*-darwin* )
  AC_DEFINE([PURGE_COMMAND],["purge"], [Command to purge filesystem cache])
  ;;
*-*-linux* )
  AC_DEFINE([DROP_CACHE_FILE],["/proc/sys/vm/drop_caches"],
	    [Path to filesystem cache purge interface])
  ;;
esac
AC_LANG([C++])
AC_PROG_CXX
AC_PROG_RANLIB
AC_SET_MAKE
AC_DEFINE([_GNU_SOURCE], [1], [use GNU extensions])
if test "x$GXX" = xyes; then
  CXX="$CXX -Wall -W -Werror -Wpointer-arith -Wwrite-strings"
fi
AX_RANDOM_DEVICE
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
