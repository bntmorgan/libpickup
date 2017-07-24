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
    *headers = NULL;

    *headers = curl_slist_append(*headers, HTTP_HEADER_USER_AGENT_MAC);

    /* the DEBUGFUNCTION has no effect until we enable VERBOSE */ 
    curl_easy_setopt(*curl, CURLOPT_VERBOSE, 0L);

    // Get if we have configured a proxy
    proxy = getenv("https_proxy");


    /* send all data to this function  */
    curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, write_res);

    /* write the page body to this file handle */
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
    curl_slist_free_all(*headers); /* free the header list */

    /* always cleanup */
    curl_easy_cleanup(*curl);
    return -1;
  }

  return 0;
}

int http_curl_perform(CURL *curl, struct curl_slist *headers) {
  CURLcode res;

  /* pass our list of custom made headers */
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  /* Perform the request, res will get the return code */
  res = curl_easy_perform(curl);

  /* Check for errors */
  if(res != CURLE_OK) {
    ERROR("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    return -1;
  }

  long http_code = 0;
  curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
  if (http_code < 200 || http_code > 299) {
    curl_easy_cleanup(curl);
    ERROR("HTTP error code is not in [200; 299] : %d\n", http_code);
    return -1;
  }
  DEBUG("HTTP ERROR CODE(%ld)\n", http_code);

  curl_slist_free_all(headers); /* free the header list */

  /* always cleanup */
  curl_easy_cleanup(curl);

  return 0;
}

int http_download_file(const char *url, char **out, size_t *count) {
  CURL *curl;
  struct curl_slist *headers;
  struct context ctx;
  if (http_curl_prepare(&curl, &headers, &ctx) != 0) {
    ERROR("Failed to prepare a curl HTTP request\n");
    return -1;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url);
  if (http_curl_perform(curl, headers) != 0) {
    ERROR("Failed to download %s\n", url);
    return -1;
  }
  *out = &ctx.buf[0];
  *count = ctx.size;
  DEBUG("Downloaded %u bytes\n", *count);
  return 0;
}
