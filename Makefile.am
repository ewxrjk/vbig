#
# This file is part of vbig.
# Copyright (C) 2011, 2013-16, 2019 Richard Kettlewell
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
tag:=$(shell git describe --tags --dirty)
bin_PROGRAMS=vbig
if WANT_FAKESTICK
  noinst_LTLIBRARIES=fakestick.la
endif
noinst_PROGRAMS=t-arcfour t-aes-ctr-drbg
vbig_SOURCES=vbig.cc Rng.h Arcfour.h Arcfour.cc CtrDrbg.h CtrDrbg.cc \
	vbig.h capture.cc safepath.cc safepath_linux.cc safepath_macos.cc
vbig_LDADD=${NETTLE_LIBS} ${JSONCPP_LIBS} ${EXTRA_LIBS}
t_arcfour_SOURCES=t-arcfour.cc Arcfour.cc
t_arcfour_LDADD=${NETTLE_LIBS}
t_aes_ctr_drbg_SOURCES=t-aes-ctr-drbg.cc CtrDrbg.cc
t_aes_ctr_drbg_LDADD=${NETTLE_LIBS}
fakestick_la_SOURCES=fakestick.c
fakestick_la_LDFLAGS=-module -shared -avoid-version -rpath $(shell pwd)
AM_CXXFLAGS=${NETTLE_CFLAGS} ${JSONCPP_CFLAGS} -DTAG=\"${tag}\"
man_MANS=vbig.1
TEST_SHELLS=t-both t-seeded t-separate t-corrupt t-truncated t-extended t-args t-fakestick t-arcfour-disabled
TESTS=t-arcfour t-aes-ctr-drbg ${TEST_SHELLS}
EXTRA_DIST=${man_MANS} README.md ${TEST_SHELLS} t-fakestick-captive CHANGES.md \
  debian/changelog debian/control debian/rules debian/compat debian/copyright \
  debian/sources/format scripts/dist
ACLOCAL_AMFLAGS=-I m4
