/*
 * This file is part of vbig.
 * Copyright (C) 2016 Richard Kettlewell
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
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <nbdkit-plugin.h>

static int64_t actual_size = 1024 * 1024;
static int64_t advertized_size = 2 * 1024 * 1024;
static const char *path = "fakestick.img";
static int fd = -1;

#define THREAD_MODEL NBDKIT_THREAD_MODEL_SERIALIZE_ALL_REQUESTS

static int64_t min(int64_t a, int64_t b) {
  return a < b ? a : b;
}

static int fakestick_config(const char *key, const char *value) {
  if(!strcmp(key, "actual_size"))
    actual_size = nbdkit_parse_size(value);
  else if(!strcmp(key, "advertized_size"))
    advertized_size = nbdkit_parse_size(value);
  else if(!strcmp(key, "path"))
    path = strdup(value);
  else {
    nbdkit_error("unrecognized fakestick configuration key '%s'", key);
    return -1;
  }
  return 0;
}

static int fakestick_config_complete(void) {
  assert(fd == -1);
  if((fd = open(path, O_RDWR|O_CREAT, 0666)) < 0) {
    nbdkit_error("opening backing store: %s", strerror(errno));
    return -1;
  }
  if(ftruncate(fd, actual_size) < 0) {
    nbdkit_error("resizing backing store: %s", strerror(errno));
    return -1;
  }
  return 0;
}

static void *fakestick_open(int readonly) {
  int *handle = malloc(1);
  if(!handle)
    nbdkit_error("creating handle: %s", strerror(errno));
  return handle;
}

static void fakestick_close(void *handle) {
  free(handle);
}

static int64_t fakestick_get_size(void *handle) {
  return advertized_size;
}

#define FAKESTICK_RW(OP)                                                \
do {                                                                    \
  while(count > 0) {                                                    \
    offset = offset % actual_size;                                      \
    uint32_t chunk = min(count, actual_size - offset);                  \
    assert(offset < advertized_size);                                   \
    assert(chunk < advertized_size);                                    \
    assert(chunk <= advertized_size - offset);                          \
    int n = OP(fd, buf, chunk, offset);                                 \
    if(n < 0) {                                                         \
      nbdkit_error("backing store I/O: %s", strerror(errno));           \
      return -1;                                                        \
    }                                                                   \
    if((unsigned)n < chunk) {                                           \
      nbdkit_error("backing store I/O: short");                         \
      return -1;                                                        \
    }                                                                   \
    offset += chunk;                                                    \
    count -= chunk;                                                     \
    buf = (void *)((char *)buf + chunk);                                \
  }                                                                     \
  return 0;                                                             \
} while(0)

static int fakestick_pread(void *handle, void *buf,
                           uint32_t count, uint64_t offset) {
  FAKESTICK_RW(pread);
}

static int fakestick_pwrite(void *handle, const void *buf,
                            uint32_t count, uint64_t offset) {
  FAKESTICK_RW(pwrite);
}

static struct nbdkit_plugin fakestick_plugin = {
  .name              = "fakestick",
  .version           = VERSION,
  .longname          = "Fake USB stick",
  .description       = "Acts like a USB stick which mis-advertizes its size",
  .config            = fakestick_config,
  .config_complete   = fakestick_config_complete,
  .open              = fakestick_open,
  .close             = fakestick_close,
  .get_size          = fakestick_get_size,
  .pread             = fakestick_pread,
  .pwrite            = fakestick_pwrite,
  /* etc */
};

NBDKIT_REGISTER_PLUGIN(fakestick_plugin)


