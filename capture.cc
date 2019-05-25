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

// Run a subprocess and capture its stdout
void capture(std::string &output, const char *file, const char **args) {
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
  case -1: fatal(errno, "fork");
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
