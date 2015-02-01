//-*-C++-*-
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
#ifndef ARCFOUR_H
# define ARCFOUR_H

#include "Rng.h"
#include <nettle/arcfour.h>

class Arcfour: public Rng {
  struct arcfour_ctx ctx;
public:
  virtual void seed(const uint8_t *key, size_t keylen);
  void stream(uint8_t *outbuf, size_t length);
};

class ArcfourDrop3072: public Arcfour {
public:
  void seed(const uint8_t *key, size_t keylen);
};

#endif /* ARCFOUR_H */
