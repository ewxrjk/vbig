vbig
====

vbig creates or verifies a file full of psuedo-random data.
The intended use is to verify that storage media work properly.

Installation
------------

If you got it from git:

    autoreconf -is

In any case:

    ./configure
    make
    sudo make install

You will need the Nettle library of cryptographic primitives:
  http://www.lysator.liu.se/~nisse/nettle/

Author
------

Copyright (C) 2011, 2013-2015 Richard Kettlewell

Copyright (C) 2013 Ian Jackson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
