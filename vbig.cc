#include <config.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <cstdarg>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include "Arcfour.h"

// Path to magic file to drop filesystem caches
static const char dropCaches[] = "/proc/sys/vm/drop_caches";

// Command line options
const struct option opts[] = {
  { "seed", required_argument, 0, 's' },
  { "verify", no_argument, 0, 'v' },
  { "create", no_argument, 0, 'c' },
  { "flush", no_argument, 0, 'f' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { 0, 0, 0, 0 },
};

// Display help message
static void help(void) {
  printf("vbig - create or verify a large but pseudo-random file\n"
         "\n"
         "Usage:\n"
         "  vbig [--seed SEED] --verify|--create PATH SIZE\n"
         "\n"
         "Options:\n"
         "  --seed         Specify random seed\n"
         "  --verify       Verify that PATH contains the expected contents\n"
         "  --create       Create PATH with psuedo-random contents\n"
         "  --flush        Flush cache\n"
         "  --help         Display usage message\n"
         "  --version      Display version string\n");
}

// Possible modes of operation
enum mode_type {
  NONE,
  VERIFY,
  CREATE
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
  int fd;
  if((fd = open(dropCaches, O_WRONLY, 0)) < 0)
    fatal(errno, "%s", dropCaches);
  if(write(fd, "3\n", 2) < 0)
    fatal(errno, "%s", dropCaches);
  close(fd);
}

int main(int argc, char **argv) {
  const char *seed = "hexapodia as the key insight";
  mode_type mode = NONE;
  bool flush = false;
  int n;
  while((n = getopt_long(argc, argv, "+s:vcfhV", opts, 0)) >= 0) {
    switch(n) {
    case 's': seed = optarg; break;
    case 'v': mode = VERIFY; break;
    case 'c': mode = CREATE; break;
    case 'f': flush = true; break;
    case 'h': help(); exit(0);
    case 'V': puts(VERSION); exit(0);
    default:
      fatal(0, "unknown option");
    }
  }
  if(mode == NONE) {
    fatal(0, "must specify one of --verify or --create");
    exit(1);
  }
  if(optind + 2 != argc) {
    fatal(0, "must specify a path and size");
    exit(1);
  }
  const char *path = argv[optind];
  errno = 0;
  char *end;
  long long size = strtoll(argv[optind + 1], &end, 10);
  if(errno) {
    fatal(errno, "invalid size");
    exit(1);
  }
  if(end == argv[optind + 1]) {
    fatal(0, "invalid size");
    exit(1);
  }
  if(!strcmp(end, "K"))
    size *= 1024;
  else if(!strcmp(end, "M"))
    size *= 1024 * 1024;
  else if(!strcmp(end, "G"))
    size *= 1024 * 1024 * 1024;
  else if(*end) {
    fatal(0, "invalid size");
    exit(1);
  } 
  Arcfour rng(seed, strlen(seed));
  FILE *fp = fopen(path, mode == VERIFY ? "rb" : "wb");
  if(!fp) {
    fatal(errno, "%s", path);
    exit(1);
  }
  if(mode == VERIFY && flush)
    flushCache(fp);
  char generated[4096], input[4096];
  size_t remain = size;
  while(remain > 0) {
    size_t bytesGenerated = (remain > sizeof generated
                             ? sizeof generated
                             : remain);
    rng.stream(generated, bytesGenerated);
    if(mode == CREATE) {
      fwrite(generated, 1, bytesGenerated, fp);
      if(ferror(fp)) {
        fatal(errno, "%s", path);
        exit(1);
      }
    } else {
      size_t bytesRead = fread(input, 1, bytesGenerated, fp);
      if(ferror(fp)) {
        fatal(errno, "%s", path);
        exit(1);
      }
      if(bytesRead < bytesGenerated) {
        fatal(0, "%s: truncated at %lld/%lld bytes",
                path, (size - remain + bytesRead), size);
        exit(1);
      }
      if(memcmp(generated, input, bytesGenerated)) {
        for(size_t n = 0; n < bytesGenerated; ++n)
          if(generated[n] != input[n]){
            fatal(0, "%s corrupted at %lld/%lld bytes (expected %d got %d)",
                    path, size - remain + n, size,
                    (unsigned char)generated[n], (unsigned char)input[n]);
            exit(1);
          }
      }
    }
    remain -= bytesGenerated;
  }
  if(mode == VERIFY && getc(fp) != EOF) {
    fatal(0, "%s: extended beyond %lld bytes",
            path, size);
    exit(1);
  }
  if(mode == CREATE && flush) {
    if(fflush(fp) < 0)
      fatal(errno, "%s", path);
    flushCache(fp);
  }
  if(fclose(fp) < 0) {
    fatal(errno, "%s", path);
    exit(1);
  }
  return 0;
}
