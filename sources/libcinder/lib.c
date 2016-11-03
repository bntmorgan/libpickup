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
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cinder/cinder.h>

#include "api.h"
#include "parser.h"
#include "log.h"

#define TMP_TEMPLATE "/tmp/cinder_XXXXXX"

#define HTTP_HEADER_USER_AGENT_MAC "User-Agent: Tinder/3.0.4 "\
  "(iPhone; iOS 7.1; Scale/2.00)"

struct context {
  // Error core
  int error_code;
  // Response buffer
  char *buf;
  // Size
  size_t size;
};

static const char *at;

void cinder_set_access_token(const char *access_token) {
  at = access_token;
}

void cinder_init(void) {
  curl_global_init(CURL_GLOBAL_ALL);
}

void cinder_cleanup(void) {
  curl_global_cleanup();
}

static size_t write_res(void *ptr, size_t size, size_t nmemb, struct context
    *ctx) {
  ctx->size += nmemb * size;
  ctx->buf = realloc(ctx->buf, ctx->size);
  if (ctx->buf == NULL) {
    ctx->error_code = CINDER_ERR_NO_MEM;
    return 0;
  }
  strncat(ctx->buf, ptr, nmemb * size);
  return nmemb * size;
}

int prepare_curl(CURL **curl, struct curl_slist **headers,
    struct context *ctx) {
  char *proxy;

  /* get a curl handle */
  *curl = curl_easy_init();
  if(*curl) {
    *headers = NULL;

    *headers = curl_slist_append(*headers, "Content-Type: application/json");
    *headers = curl_slist_append(*headers, HTTP_HEADER_USER_AGENT_MAC);

    /* the DEBUGFUNCTION has no effect until we enable VERBOSE */ 
    curl_easy_setopt(*curl, CURLOPT_VERBOSE, 0L);

    /* send all data to this function  */
    curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, write_res);

    // Get if we have configured a proxy
    proxy = getenv("https_proxy");

    if (proxy != NULL) {
      NOTE("With https_proxy\n");
      curl_easy_setopt(*curl, CURLOPT_PROXY, proxy);
    } else {
      NOTE("No https_proxy\n");
    }

    /* write the page body to this file handle */
    curl_easy_setopt(*curl, CURLOPT_WRITEDATA, ctx);

    // Prepare JSON parser context
    ctx->error_code = 0;
    ctx->size = 1;
    ctx->buf = malloc(ctx->size);
    ctx->buf[0] = '\0';

  } else {
    curl_slist_free_all(*headers); /* free the header list */

    /* always cleanup */
    curl_easy_cleanup(*curl);
    return -1;
  }

  return 0;
}

int perform_curl(CURL *curl, struct curl_slist *headers) {
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

int cinder_authenticate(const char *fb_access_token, char *access_token) {
  CURL *curl;
  struct curl_slist *headers;
  char data[0x1000];
  struct context ctx;

  if (fb_access_token == NULL) {
    return CINDER_ERR_NO_FB_ACCESS_TOKEN;
  }

  if (prepare_curl(&curl, &headers, &ctx) != 0) {
    return -1;
  }

  // Authenticate, get the tinder access token
  sprintf(data, "{\"facebook_token\": \"%s\"}", fb_access_token);

  curl_easy_setopt(curl, CURLOPT_URL, API_HOST API_AUTH);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

  if (perform_curl(curl, headers) != 0) {
    return -1;
  }

  // Check results
  if (ctx.error_code != CINDER_OK) {
    return -1;
  }

  // End the string correctly
  ctx.size += 1;
  ctx.buf = realloc(ctx.buf, ctx.size);
  ctx.buf[ctx.size - 1] = '\0';

  // Parse the received document
  if (parser_token(ctx.buf, access_token) != 0) {
    return -1;
  }

  // Free buffer
  free(ctx.buf);

  return 0;
}

int cinder_updates(struct cinder_updates_callbacks *cb, void *data,
    int *last_activity_date) {
  CURL *curl;
  struct curl_slist *headers;
  struct context ctx;

  if (at == NULL) {
    return CINDER_ERR_NO_ACCESS_TOKEN;
  }

  if (prepare_curl(&curl, &headers, &ctx) != 0) {
    return -1;
  }

  // Access token header
  char b[256];
  sprintf(b, "X-Auth-Token: %s", at);
  headers = curl_slist_append(headers, b);

  curl_easy_setopt(curl, CURLOPT_URL, API_HOST API_UPDATES);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"last_activity_date\": \"\"}");

  if (perform_curl(curl, headers) != 0) {
    return -1;
  }

  // Check results
  if (ctx.error_code != CINDER_OK) {
    return -1;
  }

  // End the string correctly
  ctx.size += 1;
  ctx.buf = realloc(ctx.buf, ctx.size);
  ctx.buf[ctx.size - 1] = '\0';

  // Print the buffer
  DEBUG("DATA\n\n%s\n\nEND DATA\n", ctx.buf);

  // Parse the received document
  if (parser_updates(ctx.buf, cb, data, last_activity_date) != 0) {
    free(ctx.buf);
    return -1;
  }

  // Free buffer
  free(ctx.buf);

  return 0;
}

void cinder_match_free(struct cinder_match *m) {
  parser_match_free(m);
}

int cinder_swipe(const char *pid, int like, unsigned int *remaining_likes) {
  CURL *curl;
  struct curl_slist *headers;
  struct context ctx;

  if (at == NULL) {
    return CINDER_ERR_NO_ACCESS_TOKEN;
  }

  if (prepare_curl(&curl, &headers, &ctx) != 0) {
    return -1;
  }

  // Access token header
  char b[0x100];
  sprintf(b, "X-Auth-Token: %s", at);
  headers = curl_slist_append(headers, b);

  // Create the like or unlike url
  char url[0x100];
  char *api;

  if (like) {
    api = API_LIKE;
  } else {
    api = API_UNLIKE;
  }

  sprintf(url, "%s%s/%s", API_HOST, api, pid);

  DEBUG("swipe url dudes : %s\n", url);

  curl_easy_setopt(curl, CURLOPT_URL, url);

  if (perform_curl(curl, headers) != 0) {
    return -1;
  }

  // Check results
  if (ctx.error_code != CINDER_OK) {
    return -1;
  }

  // End the string correctly
  ctx.size += 1;
  ctx.buf = realloc(ctx.buf, ctx.size);
  ctx.buf[ctx.size - 1] = '\0';

  // Print the buffer
  DEBUG("DATA\n\n%s\n\nEND DATA\n", ctx.buf);

  // Parse the received document
  if (parser_swipe(ctx.buf, remaining_likes) != 0) {
    free(ctx.buf);
    return -1;
  }

  // Free buffer
  free(ctx.buf);

  return 0;
}

int cinder_recs(struct cinder_recs_callbacks *cb, void *data) {
  CURL *curl;
  struct curl_slist *headers;
  struct context ctx;

  if (at == NULL) {
    return CINDER_ERR_NO_ACCESS_TOKEN;
  }

  if (prepare_curl(&curl, &headers, &ctx) != 0) {
    return -1;
  }

  // Access token header
  char b[256];
  sprintf(b, "X-Auth-Token: %s", at);
  headers = curl_slist_append(headers, b);

  curl_easy_setopt(curl, CURLOPT_URL, API_HOST API_RECS);

  if (perform_curl(curl, headers) != 0) {
    return -1;
  }

  // Check results
  if (ctx.error_code != CINDER_OK) {
    return -1;
  }

  // End the string correctly
  ctx.size += 1;
  ctx.buf = realloc(ctx.buf, ctx.size);
  ctx.buf[ctx.size - 1] = '\0';

  // Print the buffer
  // fprintf(stdout, "DATA\n\n%s\n\nEND DATA\n", ctx.buf);

  // Parse the received document
  if (parser_recs(ctx.buf, cb, data) != 0) {
    free(ctx.buf);
    return -1;
  }

  // Free buffer
  free(ctx.buf);

// XXX do not loose experiment data ! These are real poor girls !
//  char *buf = malloc(0x20000);
//  FILE *fd = fopen("JS-recs.beauty", "r");
//  fread(buf, 1, 0x20000, fd);
//  if (parser_recs(buf, cb, data) != 0) {
//    free(buf);
//    return -1;
//  }
//  free(buf);
  return 0;
}

int cinder_message(const char *mid, const char *message) {
  CURL *curl;
  struct curl_slist *headers;
  struct context ctx;

  if (at == NULL) {
    return CINDER_ERR_NO_ACCESS_TOKEN;
  }

  if (prepare_curl(&curl, &headers, &ctx) != 0) {
    return -1;
  }

  // Access token header
  char b[256];
  sprintf(b, "X-Auth-Token: %s", at);
  headers = curl_slist_append(headers, b);

  char url[0x100];
  sprintf(url, "%s%s/%s", API_HOST, API_MESSAGE, mid);

  printf("message url dudes : %s\n", url);

  char post_data[CINDER_SIZE_MESSAGE];
  sprintf(post_data, "{\"message\":\"%s\"}", message);

  printf("Data to post %s\n", post_data);

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

  if (perform_curl(curl, headers) != 0) {
    return -1;
  }

  // Check results
  if (ctx.error_code != CINDER_OK) {
    return -1;
  }

  // End the string correctly
  ctx.size += 1;
  ctx.buf = realloc(ctx.buf, ctx.size);
  ctx.buf[ctx.size - 1] = '\0';

  // Print the buffer
  fprintf(stdout, "DATA\n\n%s\n\nEND DATA\n", ctx.buf);

  // Parse the received document
//  if (parser_updates(ctx.buf) != 0) {
//    free(ctx.buf);
//    return -1;
//  }

  // Free buffer
  free(ctx.buf);

  return 0;
}

void cinder_log_level(int l) {
  log_level(l);
}

void cinder_match_print(struct cinder_match *m) {
  int i, j;
  printf("mid(%s)\n", m->mid);
  printf("pid(%s)\n", m->pid);
  printf("name(%s)\n", m->name);
  printf("birth(%ld)\n", m->birth);
  for (i = 0; i < m->images_count; i++) {
    struct cinder_image *p = &m->images[i];
    printf("url(%s)\n", p->url);
    for (j = 0; j < 4; j++) {
      printf("width(%d), height(%d), url(%s)\n", p->processed[j].width,
          p->processed[j].height, p->processed[j].url);
    }
  }
  for (i = 0; i < m->messages_count; i++) {
    struct cinder_message *p = &m->messages[i];
    if (p->dir == CINDER_MESSAGE_INPUT) {
      printf("she :\n");
    } else {
      printf("me :\n");
    }
    printf("%s\n", p->message);
  }
}
