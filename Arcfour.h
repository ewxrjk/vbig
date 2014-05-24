//-*-C++-*-
/* arcfour.h --- The arcfour stream cipher
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005
 *    Free Software Foundation, Inc.
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
/* Code from Libgcrypt adapted for gnulib by Simon Josefsson. */

#ifndef ARCFOUR_H
# define ARCFOUR_H

# include <stddef.h>
# include <stdint.h>

#define ARCFOUR_SBOX_SIZE 256

class Arcfour {
  uint8_t sbox[ARCFOUR_SBOX_SIZE];
  uint8_t idx_i, idx_j;
public:
  Arcfour(const uint8_t *key, size_t keylen);
  void stream(uint8_t *outbuf, size_t length);
};

#endif /* ARCFOUR_H */
