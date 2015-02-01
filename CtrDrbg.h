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

#ifndef CTRDRBG_H
# define CTRDRBG_H

#include "Rng.h"
#include <nettle/aes.h>

class CtrDrbg: public Rng {
private:
  static const size_t OUTLEN_MAX = 16;
  static const size_t KEYLEN_MAX = 32;
  static const size_t SEEDLEN_MAX = OUTLEN_MAX + KEYLEN_MAX;

  uint8_t v[OUTLEN_MAX];

  void update(size_t outlen, size_t keylen,
              const uint8_t *provided_data);

  virtual void setkey(const uint8_t *key) = 0;
  virtual void encrypt(const uint8_t *input, uint8_t *output) = 0;
  static void incr(uint8_t *v, size_t vlen);
protected:
  void generate(size_t outlen, size_t keylen,
                uint8_t *outbuf, size_t length);
  void instantiate(size_t outlen, size_t keylen,
                   const uint8_t *entropy_input,
                   const uint8_t *personalization_string,
                   size_t len_personalization_string);
};

class AesCtrDrbg128: public CtrDrbg {
  struct aes_ctx ctx;
  void setkey(const uint8_t *key);
  void encrypt(const uint8_t *input, uint8_t *output);
public:
  void seed(const uint8_t *key, size_t keylen);
  void stream(uint8_t *outbuf, size_t length);
  void instantiate(const uint8_t *entropy_input,
                   const uint8_t *personalization_string,
                   size_t len_personalization_string);
};

class AesCtrDrbg192: public CtrDrbg {
  struct aes_ctx ctx;
  void setkey(const uint8_t *key);
  void encrypt(const uint8_t *input, uint8_t *output);
public:
  void seed(const uint8_t *key, size_t keylen);
  void stream(uint8_t *outbuf, size_t length);
  void instantiate(const uint8_t *entropy_input,
                   const uint8_t *personalization_string,
                   size_t len_personalization_string);
};

class AesCtrDrbg256: public CtrDrbg {
  struct aes_ctx ctx;
  void setkey(const uint8_t *key);
  void encrypt(const uint8_t *input, uint8_t *output);
public:
  void seed(const uint8_t *key, size_t keylen);
  void stream(uint8_t *outbuf, size_t length);
  void instantiate(const uint8_t *entropy_input,
                   const uint8_t *personalization_string,
                   size_t len_personalization_string);
};

#endif /* CTRDRBG_H */
