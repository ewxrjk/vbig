/*
 * This file is part of vbig.
 * Copyright (C) 2015, 2019 Richard Kettlewell
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
#if __GNUC__ && __i386__
  if(__builtin_expect(vlen == 16, 1)) {
    __asm__("mov 12(%0),%%edx\n\t"
            "mov 8(%0),%%ecx\n\t"
            "mov 4(%0),%%eax\n\t"

            "bswap %%edx\n\t"
            "add $1,%%edx\n\t"
            "bswap %%edx\n\t"
            "mov %%edx,12(%0)\n\t"

            "mov (%0),%%edx\n\t"

            "bswap %%ecx\n\t"
            "adc $0,%%ecx\n\t"
            "bswap %%ecx\n\t"
            "mov %%ecx,8(%0)\n\t"

            "bswap %%eax\n\t"
            "adc $0,%%eax\n\t"
            "bswap %%eax\n\t"
            "mov %%eax,4(%0)\n\t"

            "bswap %%edx\n\t"
            "adc $0,%%edx\n\t"
            "bswap %%edx\n\t"
            "mov %%edx,(%0)"
            :
            : "r"(v)
            : "eax", "ecx", "edx", "cc");
    return;
  }
#endif
#if __GNUC__ && (__ARM_ARCH_6__ || __ARM_ARCH_7__)
  if(__builtin_expect(vlen == 16, 1)) {
    __asm__("ldr r3,[%0,#12]\n\t"
            "ldr r2,[%0,#8]\n\t"
            "ldr r1,[%0,#4]\n\t"

            "rev r3,r3\n\t"
            "adds r3,r3,#1\n\t"
            "rev r3,r3\n\t"
            "str r3,[%0,#12]\n\t"

            "ldr r3,[%0]\n\t"

            "rev r2,r2\n\t"
            "adcs r2,r2,#0\n\t"
            "rev r2,r2\n\t"
            "str r2,[%0,#8]\n\t"

            "rev r1,r1\n\t"
            "adcs r1,r1,#0\n\t"
            "rev r1,r1\n\t"
            "str r1,[%0,#4]\n\t"

            "rev r3,r3\n\t"
            "adc r3,r3,#0\n\t"
            "rev r3,r3\n\t"
            "str r3,[%0]\n\t"
            :
            : "r"(v)
            : "r1", "r2", "r3", "cc");
    return;
  }
#endif
#if __GNUC__ && __aarch64__
  if(__builtin_expect(vlen == 16, 1)) {
    __asm__("ldp x1,x2,[%0]\n\t"
            "rev x1,x1\n\t"
            "rev x2,x2\n\t"
            "adds x2,x2,1\n\t"
            "adcs x1,x1,xzr\n\t"
            "rev x1,x1\n\t"
            "rev x2,x2\n\t"
            "stp x1,x2,[%0]"
            :
            : "r"(v)
            : "x1", "x2", "cc");
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
