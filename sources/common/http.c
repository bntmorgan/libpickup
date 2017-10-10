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

#include <stdlib.h>
#include <string.h>

#include "http.h"
#include "log.h"

char *http_error_str[] = {
  "HTTP_OK",
  "HTTP_CURL_ERROR",
  "HTTP_NETWORK",
  "HTTP_NO_MEM",
  "HTTP_REDIRECT", // HTTP 300
  "HTTP_BAD_REQUEST", // HTTP 400
  "HTTP_UNAUTHORIZED", // HTTP 401
  "HTTP_USER_ERROR", // other 4XX HTTP error codes
  "HTTP_INTERNAL_SERVER_ERROR", // HTTP 500
  "HTTP_SERVER_ERROR", // other 500 HTTP error codes
  "HTTP_ERROR", // other HTTP errors codes
  "",
};

char *http_strerror(int e) {
  if (e >= 0 && e < HTTP_ERROR_LAST) {
    return http_error_str[e];
  } else {
    return http_error_str[HTTP_ERROR_LAST];
  }
}

static size_t write_res(void *ptr, size_t size, size_t nmemb, struct context
    *ctx) {
  unsigned int size_old = ctx->size;
  ctx->size += nmemb * size;
  ctx->buf = realloc(ctx->buf, ctx->size);
  if (ctx->buf == NULL) {
    ERROR("Failed to allocate memory for HTTP response\n");
    return 0;
  }
  DEBUG("Old size %u, New size %u\n", size_old, ctx->size);
  memcpy(ctx->buf + size_old, ptr, nmemb * size);
  return nmemb * size;
}

int http_curl_prepare(CURL **curl, struct curl_slist **headers,
    struct context *ctx) {
  char *proxy;

  /* get a curl handle */
  *curl = curl_easy_init();
  if(*curl) {
    char *error_buffer;
    *headers = NULL;

    *headers = curl_slist_append(*headers, HTTP_HEADER_USER_AGENT_MAC);

    // the DEBUGFUNCTION has no effect until we enable VERBOSE
    curl_easy_setopt(*curl, CURLOPT_VERBOSE, 0L);

    // set error buffer !
    error_buffer = malloc(CURL_ERROR_SIZE);
    if (error_buffer == NULL) {
      curl_slist_free_all(*headers);
      curl_easy_cleanup(*curl);
      return HTTP_NO_MEM;
    }
    curl_easy_setopt(*curl, CURLOPT_ERRORBUFFER, error_buffer);

    // Get if we have configured a proxy
    proxy = getenv("https_proxy");

    // send all data to this function
    curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, write_res);

    // write the page body to this file handle
    curl_easy_setopt(*curl, CURLOPT_WRITEDATA, ctx);

    // Prepare context
    memset(ctx, 0, sizeof(struct context));

    if (proxy != NULL) {
      NOTE("With https_proxy\n");
      curl_easy_setopt(*curl, CURLOPT_PROXY, proxy);
    } else {
      NOTE("No https_proxy\n");
    }

  } else {
    curl_slist_free_all(*headers);
    curl_easy_cleanup(*curl);
    return HTTP_CURL_ERROR;
  }

  return HTTP_OK;
}

int http_curl_perform(CURL *curl, struct curl_slist *headers) {
  CURLcode res;

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  res = curl_easy_perform(curl);

  if(res != CURLE_OK) {
    ERROR("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    if (res >= CURLE_COULDNT_RESOLVE_PROXY &&
        res <= CURLE_REMOTE_ACCESS_DENIED) {
      return HTTP_NETWORK;
    } else {
      return HTTP_CURL_ERROR;
    }
  }

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  DEBUG("HTTP ERROR CODE(%ld)\n", http_code);

  if (http_code < 200 || http_code > 299) {
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    ERROR("HTTP error code is not in [200; 299] : %d\n", http_code);
    if (http_code >= 300 && http_code < 400) {
      return HTTP_REDIRECT;
    } else if (http_code >= 400 && http_code < 500) {
      if (http_code == 400) {
        return HTTP_BAD_REQUEST;
      } else if (http_code == 401) {
        return HTTP_UNAUTHORIZED;
      }
      return HTTP_USER_ERROR;
    } else if (http_code >= 500 && http_code < 600) {
      if (http_code == 500) {
        return HTTP_INTERNAL_SERVER_ERROR;
      }
      return HTTP_SERVER_ERROR;
    }
    return HTTP_ERROR;
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  return HTTP_OK;
}

int http_download_file(const char *url, char **out, size_t *count) {
  CURL *curl;
  struct curl_slist *headers;
  struct context ctx;
  int ret = http_curl_prepare(&curl, &headers, &ctx);
  if (ret != HTTP_OK) {
    ERROR("Failed to prepare a curl HTTP request\n");
    return -1;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url);
  ret = http_curl_perform(curl, headers);
  if (ret != HTTP_OK) {
    ERROR("Failed to download %s\n", url);
    return ret;
  }
  *out = &ctx.buf[0];
  *count = ctx.size;
  DEBUG("Downloaded %u bytes\n", *count);
  return HTTP_OK;
}
