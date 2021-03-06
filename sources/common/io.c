/*
Copyright (C) 2016  Benoît Morgan

This file is part of libpickup.

libpickup is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

libpickup is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with libpickup.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * Reads and write into the user .config/pickup directory
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

#include "io.h"
#include "log.h"

int path_resolve(const char *filename, int type, char *path, size_t n) {
  const char *homedir;
  char *sub;

  // Initialize string
  path[0] = '\0';

  if ((homedir = getenv("HOME")) == NULL) {
    homedir = getpwuid(getuid())->pw_dir;
  }

  if (type == IO_PATH_CACHE) {
    sub = IO_CACHE_DIR;
  } else if (type ==IO_PATH_CACHE_IMG) {
    sub = IO_CACHE_IMG_DIR;
  } else {
    sub = IO_CONFIG_DIR;
  }

  // Start with the homedire path
  if (snprintf(path, n, "%s/%s/%s", homedir, sub, filename) < 0) {
    ERROR("unable to generate the path : %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int file_unlink(char *filename, int type) {
  char path[0x1000];
  if (filename == NULL) {
    return -1;
  }
  if (path_resolve(filename, type, &path[0], 0x1000)) {
    return -1;
  }
  DEBUG("Unlink path : %s\n", &path[0]);
  if (unlink(&path[0]) != 0) {
    ERROR("unable to unlink file %s : %s\n", &path[0], strerror(errno));
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
  if (path_resolve(filename, IO_PATH_CONFIG, &path[0], 0x1000)) {
    return -1;
  }
  DEBUG("Write path : %s\n", &path[0]);
  out = fopen(path, "w");
  if (out == NULL) {
    ERROR("unable to open file %s : %s\n", &path[0], strerror(errno));
    return -1;
  }
  if (fputs(buf, out) == EOF) {
    ERROR("unable to write to file %s : %s\n", &path[0], strerror(errno));
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
  if (path_resolve(filename, IO_PATH_CONFIG, &path[0], 0x1000)) {
    return -1;
  }
  DEBUG("read from path : %s\n", &path[0]);
  in = fopen(path, "r");
  if (in == 0x0) {
    ERROR("unable to open file %s : %s\n", &path[0], strerror(errno));
    return -1;
  }
  if (fgets(buf, count, in) == NULL) {
    ERROR("unable to read from file %s : %s\n", &path[0], strerror(errno));
    return -1;
  }
  fclose(in);
  return 0;
}

int file_write(char *filename, int type, char *buf, size_t count) {
  FILE *out;
  char path[0x1000];
  if (filename == NULL) {
    return -1;
  }
  if (path_resolve(filename, type, &path[0], 0x1000)) {
    return -1;
  }
  DEBUG("Write path : %s\n", &path[0]);
  out = fopen(path, "w");
  if (out == NULL) {
    ERROR("unable to open file %s : %s\n", &path[0], strerror(errno));
    return -1;
  }
  if (fwrite(buf, 1, count, out) == EOF) {
    ERROR("unable to write to file %s : %s\n", &path[0], strerror(errno));
    return -1;
  }

  fclose(out);
  return 0;
}

int file_exists(char *filename, int type) {
  char path[0x1000];
  if (filename == NULL) {
    return -1;
  }
  if (path_resolve(filename, type, &path[0], 0x1000)) {
    return -1;
  }
  DEBUG("Access path : %s\n", &path[0]);
  if (access(path, R_OK) != 0) {
    DEBUG("unable to open access %s : %s\n", &path[0], strerror(errno));
    return -1;
  }
  return 0;
}
