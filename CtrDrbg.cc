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
#include <config.h>
#include "CtrDrbg.h"
#include <cstring>
#include <algorithm>

void CtrDrbg::incr(uint8_t *v, size_t vlen) {
#if __GNUC__ && __amd64__
  if(__builtin_expect(vlen == 16, 1)) {
    __asm__("mov 8(%0),%%rcx\n\t"
            "mov (%0),%%rax\n\t"
            "bswap %%rcx\n\t"
            "bswap %%rax\n\t"
            "add $1,%%rcx\n\t"
            "adc $0,%%rax\n\t"
            "bswap %%rcx\n\t"
            "bswap %%rax\n\t"
            "mov %%rcx,8(%0)\n\t"
            "mov %%rax,(%0)"
            :
            : "r"(v)
            : "rax", "rcx", "cc");
    return;
  }
#endif
  v += vlen;
  int carry = 1;
  while(vlen > 0) {
    int i = *--v + carry;
    *v = i;
    carry = i >> 8;
    vlen--;
  }
}

void CtrDrbg::update(size_t outlen, size_t keylen,
                     const uint8_t *provided_data) {
  uint8_t temp[OUTLEN_MAX + KEYLEN_MAX];
  const size_t seedlen = outlen + keylen;
  size_t templen = 0;
  while(templen < seedlen) {
    incr(v, outlen);
    encrypt(v, temp + templen);
    templen += outlen;
  }
  for(size_t i = 0; i < seedlen; ++i)
    temp[i] ^= provided_data[i];
  setkey(temp);
  memcpy(v, temp + keylen, outlen);
}

void CtrDrbg::instantiate(size_t outlen, size_t keylen,
                          const uint8_t *entropy_input,
                          const uint8_t *personalization_string,
                          size_t len_personalization_string) {
  uint8_t seed_material[SEEDLEN_MAX];
  uint8_t key[KEYLEN_MAX];
  const size_t seedlen = outlen + keylen;
  memcpy(seed_material, entropy_input, seedlen);
  if(len_personalization_string > seedlen)
    len_personalization_string = seedlen;
  for(size_t i = 0; i < len_personalization_string; ++i)
    seed_material[i] ^= personalization_string[i];
  memset(key, 0, keylen);
  memset(v, 0, outlen);
  setkey(key);
  update(outlen, keylen, seed_material);
}

void CtrDrbg::generate(size_t outlen, size_t keylen,
                       uint8_t *outbuf, size_t length) {
  static const uint8_t zero[SEEDLEN_MAX] = { 0 };
  uint8_t output_block[OUTLEN_MAX];
  memset(outbuf, 'A', length);  // TODO remove
  while(length > 0) {
    incr(v, outlen);
    encrypt(v, output_block);
    size_t chunk = std::min(length, outlen);
    memcpy(outbuf, output_block, chunk);
    outbuf += chunk;
    length -= chunk;
  }
  update(outlen, keylen, zero);
}

// AesCtrDrbg128

void AesCtrDrbg128::setkey(const uint8_t *key) {
  aes_set_encrypt_key(&ctx, 128/8, key);
}

void AesCtrDrbg128::encrypt(const uint8_t *input, uint8_t *output) {
  aes_encrypt(&ctx, AES_BLOCK_SIZE, output, input);
}

void AesCtrDrbg128::instantiate(const uint8_t *entropy_input,
                                const uint8_t *personalization_string,
                                size_t len_personalization_string) {
  CtrDrbg::instantiate(AES_BLOCK_SIZE, 128/8,
                       entropy_input,
                       personalization_string,
                       len_personalization_string);
}

void AesCtrDrbg128::stream(uint8_t *outbuf, size_t length) {
  generate(AES_BLOCK_SIZE, 128/8, outbuf, length);
}

void AesCtrDrbg128::seed(const uint8_t *keybytes, size_t keybyteslen) {
  static const uint8_t zero[AES_BLOCK_SIZE+128/8] = { 0 };
  instantiate(zero,
              keybytes, keybyteslen);
}

// AesCtrDrbg192

void AesCtrDrbg192::setkey(const uint8_t *key) {
  aes_set_encrypt_key(&ctx, 192/8, key);
}

void AesCtrDrbg192::encrypt(const uint8_t *input, uint8_t *output) {
  aes_encrypt(&ctx, AES_BLOCK_SIZE, output, input);
}

void AesCtrDrbg192::instantiate(const uint8_t *entropy_input,
                                const uint8_t *personalization_string,
                                size_t len_personalization_string) {
  CtrDrbg::instantiate(AES_BLOCK_SIZE, 192/8,
                       entropy_input,
                       personalization_string,
                       len_personalization_string);
}

void AesCtrDrbg192::stream(uint8_t *outbuf, size_t length) {
  generate(AES_BLOCK_SIZE, 192/8, outbuf, length);
}

void AesCtrDrbg192::seed(const uint8_t *keybytes, size_t keybyteslen) {
  static const uint8_t zero[AES_BLOCK_SIZE+192/8] = { 0 };
  instantiate(zero,
              keybytes, keybyteslen);
}

// AesCtrDrbg256

void AesCtrDrbg256::setkey(const uint8_t *key) {
  aes_set_encrypt_key(&ctx, 256/8, key);
}

void AesCtrDrbg256::encrypt(const uint8_t *input, uint8_t *output) {
  aes_encrypt(&ctx, AES_BLOCK_SIZE, output, input);
}

void AesCtrDrbg256::instantiate(const uint8_t *entropy_input,
                                const uint8_t *personalization_string,
                                size_t len_personalization_string) {
  CtrDrbg::instantiate(AES_BLOCK_SIZE, 256/8,
                       entropy_input,
                       personalization_string,
                       len_personalization_string);
}

void AesCtrDrbg256::stream(uint8_t *outbuf, size_t length) {
  generate(AES_BLOCK_SIZE, 256/8, outbuf, length);
}

void AesCtrDrbg256::seed(const uint8_t *keybytes, size_t keybyteslen) {
  static const uint8_t zero[AES_BLOCK_SIZE+256/8] = { 0 };
  instantiate(zero,
              keybytes, keybyteslen);
}
