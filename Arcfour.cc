/*
 * This file is part of vbig.
 * Copyright (C) 2011, 2014, 2015 Richard Kettlewell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <config.h>
#include <cstring>
#include "Arcfour.h"

void Arcfour::stream(uint8_t *outbuf, size_t length) {
  memset(outbuf, 0, length);
  arcfour_crypt(&ctx, length, outbuf, outbuf);
}

void Arcfour::seed(const uint8_t *key, size_t keylen) {
  arcfour_set_key(&ctx, keylen, key);
}

void ArcfourDrop3072::seed(const uint8_t *key, size_t keylen) {
  Arcfour::seed(key, keylen);
  uint8_t dropped[3072]; // en.wikipedia.org/wiki/RC4#Security
  stream(dropped, sizeof dropped);
}
