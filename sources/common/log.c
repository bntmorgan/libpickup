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
