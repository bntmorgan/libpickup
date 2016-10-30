#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

enum log_level {
  LOG_LEVEL_NONE,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_NOTE,
  LOG_LEVEL_DEBUG
};

extern FILE **file_error;
extern FILE **file_note;
extern FILE **file_debug;

#define LOG(stream, lvl, prefix, fmt, ...) \
    log_log(stream, lvl, prefix, __FILE__, __func__, __LINE__, \
    fmt , ##__VA_ARGS__);

#define ERROR(fmt, ...) \
    LOG(file_error, LOG_LEVEL_ERROR, "[error]", fmt, ##__VA_ARGS__)
#define NOTE(fmt, ...) \
    LOG(file_note, LOG_LEVEL_NOTE, "[note]", fmt, ##__VA_ARGS__)
#define DEBUG(fmt, ...) \
    LOG(file_debug, LOG_LEVEL_DEBUG, "[debug]", fmt, ##__VA_ARGS__)
#define ERROR_RAW(fmt, ...) \
    log_raw(file_error, LOG_LEVEL_ERROR, fmt , ##__VA_ARGS__);
#define DEBUG_RAW(fmt, ...) \
    log_raw(file_error, LOG_LEVEL_DEBUG, fmt , ##__VA_ARGS__);

void log_level(int l);
int log_log(FILE **stream, int l, const char *prefix, const char *file, const
    char *func, int line, const char *fmt, ...);
int log_raw(FILE **stream, int l, const char *fmt, ...);

#endif//__LOG_H__
