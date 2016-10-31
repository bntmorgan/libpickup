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

#include <stdio.h>
#include <stdarg.h>
#include "log.h"

static int lvl = LOG_LEVEL_ERROR;

FILE **file_error = &stderr;
FILE **file_note = &stdout;
FILE **file_debug = &stdout;

void log_level(int l) {
  lvl = l;
}

int log_log(FILE **stream, int l, const char *prefix, const char *file, const
    char *func, int line, const char *fmt, ...) {
  va_list arg;
  va_start(arg, fmt);
  int ret;
  if (lvl >= l) {
    fprintf(*stream, "%s%s:%d in %s(): ", prefix, file, line, func);
    ret = vfprintf(*stream, fmt, arg);
  }
  va_end(arg);
  return ret;
}

int log_raw(FILE **stream, int l, const char *fmt, ...) {
  va_list arg;
  va_start(arg, fmt);
  int ret;
  if (lvl >= l) {
    ret = vfprintf(*stream, fmt, arg);
  }
  va_end(arg);
  return ret;
}
