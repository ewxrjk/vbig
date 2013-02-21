/*
 * This file is part of vbig.
 * Copyright (C) 2011 Richard Kettlewell
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
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <cstdarg>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <assert.h>
#include <sys/stat.h>
#include "Arcfour.h"

// Command line options
const struct option opts[] = {
  { "seed", required_argument, 0, 's' },
  { "both", no_argument, 0, 'b' },
  { "verify", no_argument, 0, 'v' },
  { "create", no_argument, 0, 'c' },
  { "flush", no_argument, 0, 'f' },
  { "entire", no_argument, 0, 'e' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { 0, 0, 0, 0 },
};

// Display help message
static void help(void) {
  printf("vbig - create or verify a large but pseudo-random file\n"
         "\n"
         "Usage:\n"
         "  vbig [--seed SEED] --verify|--create PATH [SIZE]\n"
         "\n"
         "Options:\n"
         "  --seed, -s     Specify random seed\n"
         "  --verify, -v   Verify that PATH contains the expected contents\n"
         "  --create, -c   Create PATH with psuedo-random contents\n"
         "  --flush, -f    Flush cache\n"
         "  --entire, -e   Write until full; read until EOF\n"
         "  --help, -h     Display usage message\n"
         "  --version, -V  Display version string\n");
}

// Possible modes of operation
enum mode_type {
  VERIFY,
  CREATE,
  BOTH
};

// Report an error and exit
static void fatal(int errno_value, const char *fmt, ...) {
  va_list ap;
  fprintf(stderr, "ERROR: ");
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  if(errno_value)
    fprintf(stderr, ": %s", strerror(errno_value));
  fputc('\n', stderr);
  exit(1);
}

// Evict whatever FP points to from RAM
static void flushCache(FILE *fp) {
  // drop_caches only evicts clean pages, so first the target file is
  // synced.
  if(fsync(fileno(fp)) < 0)
    fatal(errno, "fsync");
#if defined DROP_CACHE_FILE
  int fd;
  if((fd = open(DROP_CACHE_FILE, O_WRONLY, 0)) < 0)
    fatal(errno, "%s", DROP_CACHE_FILE);
  if(write(fd, "3\n", 2) < 0)
    fatal(errno, "%s", DROP_CACHE_FILE);
  close(fd);
#elif defined PURGE_COMMAND
  int rc;
  if((rc = system(PURGE_COMMAND)) < 0)
    fatal(errno, "executing %s", PURGE_COMMAND);
  else if(rc) {
    if(WIFSIGNALED(rc)) {
      fprintf(stderr, "%s%s\n", 
	      strsignal(WTERMSIG(rc)),
	      WCOREDUMP(rc) ? " (core dumped)" : "");
      exit(WTERMSIG(rc) + 128);
    } else
      exit(WEXITSTATUS(rc));
  }
#endif
}

static long long execute(mode_type mode, bool entire, const char *show);

static const char default_seed[] = "hexapodia as the key insight";
static const void *seed;
static size_t seedlen;
static const char *path;
static bool entireopt = false;
static bool flush = false;
static long long size;

int main(int argc, char **argv) {
  mode_type mode = BOTH;
  int n;
  while((n = getopt_long(argc, argv, "+s:vcefhV", opts, 0)) >= 0) {
    switch(n) {
    case 's': seed = optarg; seedlen = strlen(optarg); break;
    case 'b': mode = BOTH; break;
    case 'v': mode = VERIFY; break;
    case 'c': mode = CREATE; break;
    case 'e': entireopt = true; break;
    case 'f': flush = true; break;
    case 'h': help(); exit(0);
    case 'V': puts(VERSION); exit(0);
    default:
      fatal(0, "unknown option");
    }
  }
  argc -= optind;
  argv += optind;
  if(argc > 2)
    fatal(0, "excess arguments");
  if(argc == 1 && mode == BOTH)
    entireopt = true;
  if(entireopt) {
    if(argc != 1)
      fatal(0, "with --entire, size should not be specified");
  } else {
    if(argc < (mode == VERIFY ? 1 : 2))
      fatal(0, "insufficient arguments");
  }
  if (!seed) {
    seed = default_seed;
    seedlen = sizeof(default_seed)-1;
  }
  path = argv[0];
  if(argc > 1) {
    errno = 0;
    char *end;
    size = strtoll(argv[1], &end, 10);
    if(errno)
      fatal(errno, "invalid size");
    if(end == argv[1])
      fatal(0, "invalid size");
    if(!strcmp(end, "K"))
      size *= 1024;
    else if(!strcmp(end, "M"))
      size *= 1024 * 1024;
    else if(!strcmp(end, "G"))
      size *= 1024 * 1024 * 1024;
    else if(*end)
      fatal(0, "invalid size");
  } else if(entireopt) {
    size = LONG_LONG_MAX;
  } else {
    struct stat sb;
    if(stat(path, &sb) < 0)
      fatal(errno, "stat %s", path);
    size = sb.st_size;
  }
  const char *show = entireopt ? (mode == CREATE ? "written" : "verified") : 0;
  if(mode == BOTH) {
    size = execute(CREATE, entireopt, 0);
    execute(VERIFY, false, show);
  } else {
    execute(mode, entireopt, show);
  }
  return 0;
}

static long long execute(mode_type mode, bool entire, const char *show) {
  Arcfour rng((const char*)seed, seedlen);
  FILE *fp = fopen(path, mode == VERIFY ? "rb" : "wb");
  if(!fp)
    fatal(errno, "%s", path);
  if(mode == VERIFY && flush)
    flushCache(fp);
  if(mode == CREATE && entire)
    setvbuf(fp, 0, _IONBF, 0);
  char generated[4096], input[4096];
  long long remain = size;
  static const size_t rc4drop = 3072; // en.wikipedia.org/wiki/RC4#Security
  assert(rc4drop <= sizeof(generated));
  rng.stream(generated, rc4drop);
  while(remain > 0) {
    size_t bytesGenerated = (remain > (ssize_t)sizeof generated
                             ? sizeof generated
                             : remain);
    rng.stream(generated, bytesGenerated);
    if(mode == CREATE) {
      size_t bytesWritten = fwrite(generated, 1, bytesGenerated, fp);
      if(ferror(fp)) {
	if(!entire || errno != ENOSPC)
	  fatal(errno, "%s", path);
	remain -= bytesWritten;
	break;
      }
      assert(bytesWritten == bytesGenerated);
    } else {
      size_t bytesRead = fread(input, 1, bytesGenerated, fp);
      if(ferror(fp))
        fatal(errno, "%s", path);
      if(memcmp(generated, input, bytesRead)) {
        for(size_t n = 0; n < bytesRead; ++n)
          if(generated[n] != input[n])
            fatal(0, "%s corrupted at %lld/%lld bytes (expected %d got %d)",
                    path, size - remain + n, size,
                    (unsigned char)generated[n], (unsigned char)input[n]);
      }
      if(bytesRead < bytesGenerated) {
	if(entire) {
	  assert(feof(fp));
	  remain -= bytesRead;
	  break;
	}
        fatal(0, "%s: truncated at %lld/%lld bytes",
                path, (size - remain + bytesRead), size);
      }
    }
    remain -= bytesGenerated;
  }
  if(mode == VERIFY && !entire && getc(fp) != EOF)
    fatal(0, "%s: extended beyond %lld bytes",
            path, size);
  if(mode == CREATE && flush) {
    if(fflush(fp) < 0)
      fatal(errno, "%s", path);
    flushCache(fp);
  }
  if(fclose(fp) < 0)
    fatal(errno, "%s", path);
  long long done = size - remain;
  if(show) {
    printf("%lld bytes (%lldM, %lldG) %s\n",
	   done, done >> 20, done >> 30,
	   show);
    if(ferror(stdout) || fflush(stdout))
      fatal(errno, "flush stdout");
  }
  return done;
}
