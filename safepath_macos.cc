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
#if __APPLE__
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <unistd.h>
#include <sstream>
#include <tinyxml.h>

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
  // ask diskutil about the target device
  const char *args[] = {"diskutil", "info", "-plist", path.c_str(),
                        (char *)NULL};
  std::string xml;
  capture(xml, args[0], args);
  TiXmlDocument dom;
  std::istringstream s(xml);
  s >> dom;
  TiXmlElement *root = dom.RootElement(), *rd = NULL;
  if(!root)
    fatal(0, "diskutil: xml parse failed");
  for(TiXmlElement *re = root->FirstChildElement(); re;
      re = re->NextSiblingElement()) {
    if(re->ValueStr() == "dict") {
      rd = re;
      break;
    }
  }
  if(!rd)
    fatal(0, "diskutil: did not find root <dict>");
  std::string key;
  bool foundContent = false;
  for(TiXmlElement *rde = rd->FirstChildElement(); rde;
      rde = rde->NextSiblingElement()) {
    if(rde->ValueStr() == "key")
      key = rde->GetText();
    else if(key == "Content") {
      foundContent = true;
      if(rde->ValueStr() == "string") {
        const char *t = rde->GetText();
        std::string content = t ? t : "";
        if(content != "") {
          fprintf(stderr, "WARNING: %s is in use: %s\n", path.c_str(),
                  content.c_str());
          return true;
        } else
          return false;
      } else
        fatal(0, "diskutil: unexpected element type for content: <%s>",
              rde->Value());
    }
  }
  if(!foundContent)
    fatal(0, "diskutil: did not find content key");
  return false;
}
#endif
