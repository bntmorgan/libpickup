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

#ifndef __HTTP_H__
#define __HTTP_H__

#include <curl/curl.h>

#define HTTP_HEADER_USER_AGENT_MAC "User-Agent: Tinder/3.0.4 "\
  "(iPhone; iOS 7.1; Scale/2.00)"

enum http_error_codes {
  HTTP_OK,
  HTTP_CURL_ERROR,
  HTTP_NO_MEM,
  HTTP_REDIRECT, // HTTP 300
  HTTP_BAD_REQUEST, // HTTP 400
  HTTP_UNAUTHORIZED, // HTTP 401
  HTTP_USER_ERROR, // other 4XX HTTP error codes
  HTTP_INTERNAL_SERVER_ERROR, // HTTP 500
  HTTP_SERVER_ERROR, // other 500 HTTP error codes
  HTTP_ERROR, // other HTTP errors codes
  HTTP_ERROR_LAST,
};

struct context {
  // Error core
  int error_code;
  // Response buffer
  char *buf;
  // Size
  size_t size;
};

char *http_strerror(int e);
int http_curl_prepare(CURL **curl, struct curl_slist **headers,
    struct context *ctx);
int http_curl_perform(CURL *curl, struct curl_slist *headers);
int http_download_file(const char *url, char **out, size_t *count);

#endif//__HTTP_H__
