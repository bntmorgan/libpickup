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

#ifndef __OAUTH2WEBKIT_H__
#define __OAUTH2WEBKIT_H__

enum oauth2_log_level {
  OAUTH2_LOG_LEVEL_NONE,
  OAUTH2_LOG_LEVEL_ERROR,
  OAUTH2_LOG_LEVEL_NOTE,
  OAUTH2_LOG_LEVEL_DEBUG
};

enum oauth2_error_code {
  OAUTH2_OK,
  OAUTH2_REGEX,
  OAUTH2_USER_CLOSED,
  OAUTH2_NO_MEM,
  OAUTH2_NETWORK,
  OAUTH2_NO_DATA
};

struct oauth2_context;

typedef void (*oauth2_cb)(struct oauth2_context *ctx);

struct oauth2_context {
  // Common
  char *access_token; // Allocated by the user in the sync context and by the
                      // lib in the async
  const char *url_confirm; // allocated by the user
  int error_code;
  int async; // does the lib has to manage init and qui Gtk and or not
             // in addition is the processing synchronized or asynchronized
  // Async in GtkContext
  oauth2_cb cb;
};

void oauth2_init(int *argc, char ***argv);
int oauth2_get_access_token_async(const char *url, const char *url_confirm,
    oauth2_cb cb);
int oauth2_get_access_token(const char *url, const char *url_confirm,
    char *access_token);
void oauth2_log_level(int l);

#endif//__OAUTH2WEBKIT_H__
