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
#include <sys/wait.h>
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <sstream>
#if __linux__
#include <json/json.h>
#endif
#if __APPLE__
#include <tinyxml.h>
#endif

// Run a subprocess and capture its stdout
static void capture(std::string &output, const char *file, const char **args) {
  int p[2];
  pid_t pid;
  if(pipe(p) < 0)
    fatal(errno, "create pipe");
  switch(pid = fork()) {
  case 0:
    if(dup2(p[1], 1) < 0) {
      perror("dup2 pipe");
      _exit(1);
    }
    if(close(p[0]) < 0 || close(p[1]) < 0) {
      perror("close pipe");
      _exit(1);
    }
    if(execvp(file, (char **)args) < 0)
      perror("execvp");
    _exit(1);
  case -1:
    fatal(errno, "fork");
  }
  if(close(p[1]) < 0)
    fatal(errno, "close pipe");
  char buffer[512];
  int n;
  while((n = read(p[0], buffer, sizeof buffer)) > 0
        || (n < 0 && errno == EINTR)) {
    if(n > 0)
      output.append(buffer, n);
  }
  if(n < 0)
    fatal(errno, "read pipe");
  if(close(p[0]) < 0)
    fatal(errno, "close pipe");
  int status;
  if(waitpid(pid, &status, 0) < 0)
    fatal(errno, "wait for %s", file);
  if(status)
    fatal(0, "%s failed: wait status %#x", file, status);
}

// Return true if path is a block device
static bool is_block_device(const std::string &path) {
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
static bool block_device_in_use(const std::string &path) {
#if __linux__
  // Ask lslbk about the target path
  const char *args[] = { "lsblk", "--json", path.c_str(), (char *)NULL };
  std::string json;
  capture(json, args[0], args);
  Json::Value root;
  std::istringstream s(json);
  s >> root;                    // throws on error.
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
    for(Json::Value::iterator it = children.begin();
        it != children.end(); ++it) {
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
#endif
#if __APPLE__
  // ask diskutil about the target device
  const char *args[] = { "diskutil", "info", "-plist", path.c_str(), (char *)NULL };
  std::string xml;
  capture(xml, args[0], args);
  TiXmlDocument dom;
  std::istringstream s(xml);
  s >> dom;
  // TODO error handling above?
  TiXmlElement *root = dom.RootElement(), *rd = NULL;
  for(TiXmlElement *re = root->FirstChildElement(); re; re = re->NextSiblingElement()) {
    if(re->ValueStr() == "dict") {
      rd = re;
      break;
    }
  }
  if(!rd)
    fatal(0, "diskutil: did not find root <dict>");
  std::string key;
  bool foundContent = false;
  for(TiXmlElement *rde = rd->FirstChildElement(); rde; rde = rde->NextSiblingElement()) {
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
      }
      else
        fatal(0, "diskutil: unexpected element type for content: <%s>",
              rde->Value());
    }
  }
  if(!foundContent)
    fatal(0, "diskutil: did not find content key");
#endif
  return false;
}

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
