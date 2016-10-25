/*
Copyright (C) 2016  Benoît Morgan

This file is part of libcinder.

libcinder is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

libcinder is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with libcinder.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * Reads and write into the user .config/cinder directory
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

#include "io.h"

static int path_resolve(const char *filename, char *path, size_t n) {
  const char *homedir;

  // Initialize string
  path[0] = '\0';

  if ((homedir = getenv("HOME")) == NULL) {
    homedir = getpwuid(getuid())->pw_dir;
  }

  // Start with the homedire path
  if (snprintf(path, n, "%s/%s/%s", homedir, IO_CONFIG_DIR, filename) < 0) {
    fprintf(stderr,"ERROR: unable to generate the path : %s\n",
        strerror(errno));
    return -1;
  }
  return 0;
}

int str_write(char *filename, const char *buf) {
  FILE *out;
  char path[0x1000];
  if (filename == NULL) {
    return -1;
  }
  if (path_resolve(filename, &path[0], 0x1000)) {
    return -1;
  }
  printf("Write to path : %s\n", &path[0]);
  out = fopen(path, "w");
  if (out == NULL) {
    fprintf(stderr,"ERROR: unable to open file : %s\n", strerror(errno));
    return -1;
  }
  if (fputs(buf, out) == EOF) {
    fprintf(stderr,"ERROR: unable to write to file : %s\n", strerror(errno));
    return -1;
  }

  fclose(out);
  return 0;
}

int str_read(char *filename, char *buf, size_t count) {
  FILE *in;
  char path[0x1000];
  if (filename == NULL) {
    return -1;
  }
  if (path_resolve(filename, &path[0], 0x1000)) {
    return -1;
  }
  printf("read from path : %s\n", &path[0]);
  in = fopen(path, "r");
  if (in == 0x0) {
    fprintf(stderr,"ERROR: unable to open file : %s\n", strerror(errno));
    return -1;
  }
  if (fgets(buf, count, in) == NULL) {
    fprintf(stderr,"ERROR: unable to read from file : %s\n", strerror(errno));
    return -1;
  }
  fclose(in);
  return 0;
}