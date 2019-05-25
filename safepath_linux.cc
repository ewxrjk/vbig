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
#if __linux__
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <sstream>
#include <json/json.h>

// Return true if path is a block device
bool is_block_device(const std::string &path) {
  struct stat sb;

  if(stat(path.c_str(), &sb) < 0)
    return 0;
  if(S_ISBLK(sb.st_mode))
    return true;
#if __APPLE__
  // /dev/rdisk* are character versions of the disk devices
  if(S_ISCHR(sb.st_mode) && major(sb.st_rdev) == 1)
    return true;
#endif
  return false;
}

// Return true if path is in use (it must be a block device)
bool block_device_in_use(const std::string &path) {
  // Ask lslbk about the target path
  const char *args[] = {"lsblk", "--json", path.c_str(), (char *)NULL};
  std::string json;
  capture(json, args[0], args);
  Json::Value root;
  std::istringstream s(json);
  s >> root; // throws on error.
  Json::Value &dev = root["blockdevices"][0];
  Json::Value &mountpoint = dev["mountpoint"];
  // Don't overwrite mounted filesystems
  if(!mountpoint.isNull()) {
    fprintf(stderr, "WARNING: %s is mounted on %s\n",
            mountpoint.asString().c_str(), path.c_str());
    return true;
  }
  // Don't overwrite devices with children.
  // This means devices with partition tables, LVM containers, etc.
  Json::Value &children = dev["children"];
  if(!children.isNull()) {
    // Get list of children
    std::ostringstream childs;
    bool first = true;
    for(Json::Value::iterator it = children.begin(); it != children.end();
        ++it) {
      const Json::Value &child = *it;
      if(!first)
        childs << ",";
      first = false;
      childs << child["name"].asString();
    }
    fprintf(stderr, "WARNING: %s is in use: %s\n", path.c_str(),
            childs.str().c_str());
    return true;
  }
  return false;
}
#endif
