/* arcfour.c --- The arcfour stream cipher
 * Copyright (C) 2000, 2001, 2002, 2003, 2005, 2006 Free Software
 * Foundation, Inc.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <config.h>
#include "Arcfour.h"

void Arcfour::stream(uint8_t *outbuf, size_t length) {
  uint8_t i = idx_i;
  uint8_t j = idx_j;
  
  for (; length > 0; length--) {
    char t;

    i++;
    j += sbox[i];
    t = sbox[i];
    sbox[i] = sbox[j];
    sbox[j] = t;
    *outbuf++ = (sbox[(0U + sbox[i] + sbox[j]) % ARCFOUR_SBOX_SIZE]);
  }

  idx_i = i;
  idx_j = j;
}

void Arcfour::seed(const uint8_t *key, size_t keylen) {
  size_t i, j, k;
  idx_i = idx_j = 0;
  for (i = 0; i < ARCFOUR_SBOX_SIZE; i++)
    sbox[i] = i;
  for (i = j = k = 0; i < ARCFOUR_SBOX_SIZE; i++) {
    char t;
    j = (j + sbox[i] + key[k]) % ARCFOUR_SBOX_SIZE;
    t = sbox[i];
    sbox[i] = sbox[j];
    sbox[j] = t;
    if (++k == keylen)
      k = 0;
  }
}
