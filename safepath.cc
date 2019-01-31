/*
 * This file is part of vbig.
 * Copyright (C) 2019 Richard Kettlewell
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
#include "vbig.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <cerrno>
#include <unistd.h>

// Return true if path is safe to write to
bool safe_path(const std::string &path) {
  if(path.substr(0, 4) == "/dev") {
    struct stat sb;
    if(stat(path.c_str(), &sb) < 0 && errno == ENOENT) {
      fprintf(stderr, "WARNING: attempt to use nonexistent file in /dev\n");
      return false;
    }
  }
  if(is_block_device(path) && block_device_in_use(path)) {
    return false;
  }
  return true;
}
