#include <config.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <getopt.h>
#include "Arcfour.h"

const struct option opts[] = {
  { "seed", required_argument, 0, 's' },
  { "verify", no_argument, 0, 'v' },
  { "create", no_argument, 0, 'c' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { 0, 0, 0, 0 },
};

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
         "  --help         Display usage message\n"
         "  --version      Display version string\n");
}

enum mode_type {
  NONE,
  VERIFY,
  CREATE
};

int main(int argc, char **argv) {
  const char *seed = "hexapodia as the key insight";
  mode_type mode = NONE;
  int n;
  while((n = getopt_long(argc, argv, "+s:vchV", opts, 0)) >= 0) {
    switch(n) {
    case 's': seed = optarg; break;
    case 'v': mode = VERIFY; break;
    case 'c': mode = CREATE; break;
    case 'h': help(); exit(0);
    case 'V': puts(VERSION); exit(0);
    default:
      fprintf(stderr, "ERROR: unknown option\n");
      exit(1);
    }
  }
  if(mode == NONE) {
    fprintf(stderr, "ERROR: must specify one of --verify or --create\n");
    exit(1);
  }
  if(optind + 2 != argc) {
    fprintf(stderr, "ERROR: must specify a path and size\n");
    exit(1);
  }
  const char *path = argv[optind];
  errno = 0;
  char *end;
  long long size = strtoll(argv[optind + 1], &end, 10);
  if(errno) {
    fprintf(stderr, "ERROR: invalid size: %s\n", strerror(errno));
    exit(1);
  }
  if(end == argv[optind + 1]) {
    fprintf(stderr, "ERROR: invalid size\n");
    exit(1);
  }
  if(!strcmp(end, "K"))
    size *= 1024;
  else if(!strcmp(end, "M"))
    size *= 1024 * 1024;
  else if(!strcmp(end, "G"))
    size *= 1024 * 1024 * 1024;
  else if(*end) {
    fprintf(stderr, "ERROR: invalid size\n");
    exit(1);
  } 
  Arcfour rng(seed, strlen(seed));
  FILE *fp = fopen(path, mode == VERIFY ? "rb" : "wb");
  if(!fp) {
    fprintf(stderr, "ERROR: %s: %s\n", path, strerror(errno));
    exit(1);
  }
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
        fprintf(stderr, "ERROR: %s: %s\n", path, strerror(errno));
        exit(1);
      }
    } else {
      size_t bytesRead = fread(input, 1, bytesGenerated, fp);
      if(ferror(fp)) {
        fprintf(stderr, "ERROR: %s: %s\n", path, strerror(errno));
        exit(1);
      }
      if(bytesRead < bytesGenerated) {
        fprintf(stderr, "ERROR: %s: truncated at %lld/%lld bytes\n",
                path, (size - remain + bytesRead), size);
        exit(1);
      }
      if(memcmp(generated, input, bytesGenerated)) {
        for(size_t n = 0; n < bytesGenerated; ++n)
          if(generated[n] != input[n]){
            fprintf(stderr, "ERROR: %s corrupted at %lld/%lld bytes (expected %d got %d)\n",
                    path, size - remain + n, size,
                    (unsigned char)generated[n], (unsigned char)input[n]);
            exit(1);
          }
      }
    }
    remain -= bytesGenerated;
  }
  if(mode == VERIFY && getc(fp) != EOF) {
    fprintf(stderr, "ERROR: %s: extended beyond %lld bytes\n",
            path, size);
    exit(1);
  }
  if(fclose(fp) < 0) {
    fprintf(stderr, "ERROR: %s: %s\n", path, strerror(errno));
    exit(1);
  }
  return 0;
}
