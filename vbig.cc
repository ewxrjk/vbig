/*
 * This file is part of vbig.
 * Copyright (C) 2011, 2013-2017, 2019 Richard Kettlewell
 * Copyright (C) 2013 Ian Jackson
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
#include "CtrDrbg.h"

#define DEFAULT_SEED_LENGTH 256

// Command line options
const struct option opts[] = {
  { "seed", required_argument, 0, 's' },
  { "seed-file", required_argument, 0, 'S' },
  { "seed-length", required_argument, 0, 'L' },
  { "both", no_argument, 0, 'b' },
  { "verify", no_argument, 0, 'v' },
  { "create", no_argument, 0, 'c' },
  { "flush", no_argument, 0, 'f' },
  { "entire", no_argument, 0, 'e' },
  { "progress", no_argument, 0, 'p' },
  { "rng", required_argument, 0,'r' },
  { "force", no_argument, 0, 'F' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { 0, 0, 0, 0 },
};

// Display help message
static void help(void) {
  printf("vbig - create or verify a large but pseudo-random file\n"
         "\n"
         "Usage:\n"
         "  vbig [OPTIONS] [--both|--verify|--create] PATH [SIZE]\n"
         "\n"
         "Options:\n"
         "  --seed, -s        Specify random seed as string\n"
         "  --seed-file, -S   Read random seed from (start of) this file\n"
         "  --seed-length, -L Set (maximum) seed length to read from file\n"
         "  --verify, -v      Verify that PATH contains the expected contents\n"
         "  --create, -c      Create PATH with psuedo-random contents\n"
         "  --both, -b        Do both create and verify (default)\n"
         "  --flush, -f       Flush cache\n"
         "  --entire, -e      Write until full; read until EOF\n"
         "  --progress, -p    Show progress as we go\n"
         "  --rng, -r NAME    Select RNG (arcfour, or aes-ctr-drbg-128/192/256)\n"
         "  --force, -F       Ignore warnings\n"
         "  --help, -h        Display usage message\n"
         "  --version, -V     Display version string\n");
}

// Possible modes of operation
enum mode_type {
  VERIFY,
  CREATE,
  BOTH
};

static void clearprogress();

// Report an error and exit
void __attribute__((noreturn))
fatal(int errno_value, const char *fmt, ...) {
  va_list ap;
  clearprogress();
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

static void scale(int shift, long long &value) {
  switch(shift) {
  case 'K': shift = 10; break;
  case 'M': shift = 20; break;
  case 'G': shift = 30; break;
  default: fatal(0, "invalid scale");
  }
  if(value > (LLONG_MAX >> shift))
    fatal(0, "invalid size");
  value <<= shift;
}

static long long execute(mode_type mode, bool entire, const char *show,
                         Rng *rng);

static const char default_seed[] = "hexapodia as the key insight";
static void *seed;
static size_t seedlen;
static const char *seedpath;
static const char *path;
static bool entireopt = false;
static bool flush = false;
static bool progress = false;
static long long size;

int main(int argc, char **argv) {
  mode_type mode = BOTH;
  int n;
  char *ep;
  bool force = false;
  const char *rngname = "aes-ctr-drbg-128";
  while((n = getopt_long(argc, argv, "+s:S:L:bvcepfhV", opts, 0)) >= 0) {
    switch(n) {
    case 's': seed = optarg; seedlen = strlen(optarg); break;
    case 'S': seedpath = optarg; break;
    case 'L':
      seedlen = strtoul(optarg,&ep,0);
      if(ep==optarg || *ep) fatal(0, "bad number for -S");
      break;
    case 'b': mode = BOTH; break;
    case 'v': mode = VERIFY; break;
    case 'c': mode = CREATE; break;
    case 'e': entireopt = true; break;
    case 'p': progress = true; break;
    case 'f': flush = true; break;
    case 'r': rngname = optarg; break;
    case 'h': help(); exit(0);
    case 'V': puts(VERSION " " TAG); exit(0);
    case 'F': force = true; break;
    default:
      fatal(0, "unknown option");
    }
  }
  argc -= optind;
  argv += optind;
  Rng *rng;
  if(!strcasecmp(rngname, "arcfour")) {
    fprintf(stderr, "WARNING: arcfour algorithm is insecure\n");
    rng = new Arcfour();
  } else if(!strcasecmp(rngname, "arcfour-drop-3072"))
    rng = new ArcfourDrop3072();
  else if(!strcasecmp(rngname, "aes-ctr-drbg-128"))
    rng = new AesCtrDrbg128();
  else if(!strcasecmp(rngname, "aes-ctr-drbg-192"))
    rng = new AesCtrDrbg192();
  else if(!strcasecmp(rngname, "aes-ctr-drbg-256"))
    rng = new AesCtrDrbg256();
  else
    fatal(0, "unrecognized RNG '%s'", rngname);
  /* expect PATH [SIZE] */
  if(argc > 2)
    fatal(0, "excess arguments");
  /* If --both but no SIZE, assume a block device, which is to be filled */
  if(argc == 1 && mode == BOTH)
    entireopt = true;
  if(entireopt) {
    if(argc != 1)
      fatal(0, "with --entire, size should not be specified");
  } else {
    /* --create without --entire requires PATH SIZE
     * --verify just requires PATH, SIZE is optional */
    if(argc < (mode == VERIFY ? 1 : 2))
      fatal(0, "insufficient arguments");
  }
  if(seed && seedpath)
    fatal(0, "both --seed and --seed-file specified");
  if(mode == BOTH && !seed && !seedpath) {
    /* --both and no seed specified; pick a random one */
#ifdef HAVE_RANDOM_DEVICE
    seedpath = RANDOM_DEVICE;
#else
    fatal(0, "no --seed or --seed-file specified in --both mode"
	  " and random device not supported on this system");
#endif
  }
  path = argv[0];
  if(mode != VERIFY) {
    if(!safe_path(path) && !force) {
      fatal(0, "use --force to override warnings");
      exit(1);
    }
  }
  if(seedpath) {
    if(!seedlen)
      seedlen = DEFAULT_SEED_LENGTH;
    FILE *seedfile = fopen(seedpath, "rb");
    if(!seedfile)
      fatal(errno, "open %s", seedpath);
    seed = malloc(seedlen);
    if(!seed)
      fatal(errno, "allocate seed");
    seedlen = fread(seed, 1, seedlen, seedfile);
    if(ferror(seedfile))
      fatal(errno, "read %s", seedpath);
    fclose(seedfile);
  }
  if (!seed) {
    /* No seed specified, use a constant */
    seed = (void*)default_seed;
    seedlen = sizeof(default_seed)-1;
  }
  if(argc > 1) {
    /* Explicit size specified */
    errno = 0;
    char *end;
    size = strtoll(argv[1], &end, 10);
    if(errno)
      fatal(errno, "invalid size");
    if(end == argv[1])
      fatal(0, "invalid size");
    if(*end) {
      if(end[1])
        fatal(0, "invalid scale");
      scale(*end, size);
    }
  } else if(entireopt) {
    /* Use stupidly large size as a proxy for 'infinite' */
    size = LLONG_MAX;
  } else {
    /* Retrieve size from target (which must exist) */
    struct stat sb;
    if(stat(path, &sb) < 0)
      fatal(errno, "stat %s", path);
    size = sb.st_size;
  }
  const char *show = entireopt ? (mode == CREATE ? "written" : "verified") : 0;
  if(mode == BOTH) {
    size = execute(CREATE, entireopt, 0, rng);
    execute(VERIFY, false, show, rng);
  } else {
    execute(mode, entireopt, show, rng);
  }
  delete rng; /* placate memory leak checkers */
  return 0;
}

// flush stdout, fatal on error
static void flushstdout() {
  if(ferror(stdout) || fflush(stdout))
    fatal(errno, "flush stdout");
}

// clear the progress indicator
static void clearprogress() {
  if (!progress) return;
  printf(" %-10s %*s   \r", "", (int)sizeof(long long)*4, "");
  flushstdout();
}

// update progress indicator
static void showprogress(long long amount, const char *show) {
  if (!progress) return;

  static int counter;
  if (counter++ < 1000) return;
  counter = 0;

  int triples = sizeof(amount);
  char rawbuf[triples*3 + 1];
  char outbuf[triples*4 + 1];
  snprintf(rawbuf, sizeof(rawbuf), "% *lld", (int)sizeof(rawbuf)-1, amount);
  for (int i=0; i<triples; i++) {
    outbuf[i*4] = ' ';
    memcpy(outbuf + i*4 + 1, rawbuf + i*3, 3);
  }
  outbuf[triples*4] = 0;
  printf(" %-10s %s...\r", outbuf, show);
  flushstdout();
}

// write/verify the target file
static long long execute(mode_type mode, bool entire, const char *show,
                         Rng *rng) {
  rng->seed((const uint8_t *)seed, seedlen);
  FILE *fp = fopen(path, mode == VERIFY ? "rb" : "wb");
  if(!fp)
    fatal(errno, "open %s", path);
  if(mode == VERIFY && flush)
    flushCache(fp);
  if(mode == CREATE && entire)
    setvbuf(fp, 0, _IONBF, 0);
  uint8_t generated[4096], input[4096];
  long long remain = size;
  while(remain > 0) {
    size_t bytesGenerated = (remain > (ssize_t)sizeof generated
                             ? sizeof generated
                             : remain);
    rng->stream(generated, bytesGenerated);
    if(mode == CREATE) {
      size_t bytesWritten = fwrite(generated, 1, bytesGenerated, fp);
      if(ferror(fp)) {
	if(!entire || errno != ENOSPC)
	  fatal(errno, "write %s", path);
	remain -= bytesWritten;
	break;
      }
      assert(bytesWritten == bytesGenerated);
    } else {
      size_t bytesRead = fread(input, 1, bytesGenerated, fp);
      if(ferror(fp))
        fatal(errno, "read %s", path);
      if(memcmp(generated, input, bytesRead)) {
        for(size_t n = 0; n < bytesRead; ++n)
          if(generated[n] != input[n])
            fatal(0, "%s: corrupted at %lld/%lld bytes (expected %d got %d)",
                    path, size - remain + n, size,
                    (unsigned char)generated[n], (unsigned char)input[n]);
      }
      /* Truncated */
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
    showprogress(size - remain, mode == VERIFY ? "verifying" : "writing");
  }
  clearprogress();
  if(mode == VERIFY && !entire && getc(fp) != EOF)
    fatal(0, "%s: extended beyond %lld bytes",
            path, size);
  if(mode == CREATE && flush) {
    if(fflush(fp) < 0)
      fatal(errno, "flush %s", path);
    flushCache(fp);
  }
  if(fclose(fp) < 0)
    fatal(errno, "close %s", path);
  /* Actual size written/verified */
  long long done = size - remain;
  if(show) {
    printf("%lld bytes (%lldM, %lldG) %s\n",
	   done, done >> 20, done >> 30,
	   show);
    flushstdout();
  }
  return done;
}
