#
# This file is part of vbig.
# Copyright (C) 2011, 2013, 2014, 2015 Richard Kettlewell
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
bin_PROGRAMS=vbig
noinst_PROGRAMS=t-arcfour
vbig_SOURCES=vbig.cc Rng.h Arcfour.h Arcfour.cc
t_arcfour_SOURCES=t-arcfour.cc Arcfour.cc
man_MANS=vbig.1
TESTS=t-arcfour t-both t-seeded t-separate t-corrupt t-truncated t-extended t-args
EXTRA_DIST=${man_MANS} README.md ${TESTS}
