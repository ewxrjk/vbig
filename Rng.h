//-*-C++-*-
/*
 * This file is part of vbig.
 * Copyright (C) 2015 Richard Kettlewell
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
#ifndef RNG_H
#define RNG_H

#include <stddef.h>
#include <stdint.h>

class Rng {
public:
  virtual ~Rng() {}
  virtual void seed(const uint8_t *key, size_t keylen) = 0;
  virtual void stream(uint8_t *outbuf, size_t length) = 0;
};

#endif /* RNG_H */
