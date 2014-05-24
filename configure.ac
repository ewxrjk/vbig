#
# This file is part of vbig.
# Copyright (C) 2011, 2013 Richard Kettlewell
# Copyright (C) 2013 Ian Jackson
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
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
