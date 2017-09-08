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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pickup/pickup.h>

#include "api.h"
#include "http.h"
#include "parser.h"
#include "log.h"

// Access token
static const char *at;
// User person id
static const char *pid;

const char *pickup_get_access_token(void) {
  return at;
}

const char *pickup_get_pid(void) {
  return pid;
}

void pickup_set_access_token(const char *access_token, const char *user_pid) {
  at = access_token;
  pid = user_pid;
}

int pickup_is_auth(void) {
  DEBUG("at %p, pid %p\n", at, pid);
  return at != NULL && pid != NULL;
}

void pickup_init(void) {
  curl_global_init(CURL_GLOBAL_ALL);
}

void pickup_cleanup(void) {
  curl_global_cleanup();
}

int curl_prepare(CURL **curl, struct curl_slist **headers,
    struct context *ctx) {

  if(http_curl_prepare(curl, headers, ctx) != 0) {
    return PICKUP_ERR;
  }

  *headers = curl_slist_append(*headers, "Content-Type: application/json");

  // Add the access token if it exists
  if (at != NULL) {
    // Access token header
    char b[0x100];
    sprintf(b, "X-Auth-Token: %s", at);
    *headers = curl_slist_append(*headers, b);
  }

  return 0;
}

int curl_perform(CURL *curl, struct curl_slist *headers, struct context *ctx) {

  if (http_curl_perform(curl, headers) != 0) {
    ERROR("Failed to perform HTTP request\n");
    return PICKUP_ERR;
  }

  // End the string correctly
  ctx->size += 1;
  ctx->buf = realloc(ctx->buf, ctx->size);
  ctx->buf[ctx->size - 1] = '\0';

  // Print the buffer
  DEBUG("DATA\n\n%s\n\nEND DATA\n", ctx->buf);

  return 0;
}

int pickup_auth(const char *fb_access_token, char *access_token,
    char *pid) {
  CURL *curl;
  struct curl_slist *headers;
  char data[0x1000];
  struct context ctx;

  if (fb_access_token == NULL) {
    return PICKUP_ERR_NO_FB_ACCESS_TOKEN;
  }

  if (curl_prepare(&curl, &headers, &ctx) != 0) {
    return PICKUP_ERR;
  }

  // Authenticate, get the tinder access token
  sprintf(data, "{\"facebook_token\": \"%s\"}", fb_access_token);

  curl_easy_setopt(curl, CURLOPT_URL, API_HOST API_AUTH);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

  if (curl_perform(curl, headers, &ctx) != 0) {
    free(ctx.buf);
    return PICKUP_ERR;
  }

  // Check results
  if (ctx.error_code != PICKUP_OK) {
    free(ctx.buf);
    return PICKUP_ERR;
  }

  // Parse the received document
  if (parser_auth(ctx.buf, access_token, pid) != 0) {
    free(ctx.buf);
    return PICKUP_ERR;
  }

  // Free buffer
  free(ctx.buf);

  return 0;
}

int pickup_updates(struct pickup_updates_callbacks *cb, void *data,
    char *last_activity_date) {
  CURL *curl;
  char buf[0x100];
  struct curl_slist *headers;
  struct context ctx;

  if (last_activity_date == NULL) {
    ERROR("You have to give a pointer to the in out string "
        "last_activity_date\n");
    return PICKUP_ERR;
  }

  if (pickup_is_auth() == 0) {
    ERROR("No access token given\n");
    return PICKUP_ERR_NO_ACCESS_TOKEN;
  }

  if (curl_prepare(&curl, &headers, &ctx) != 0) {
    return PICKUP_ERR;
  }

  // Create the data string
  sprintf(&buf[0], "{\"last_activity_date\": \"%s\"}", &last_activity_date[0]);

  DEBUG("Data %s\n", &buf[0]);

  curl_easy_setopt(curl, CURLOPT_URL, API_HOST API_UPDATES);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &buf[0]);

  if (curl_perform(curl, headers, &ctx) != 0) {
    return PICKUP_ERR;
  }

  // Check results
  if (ctx.error_code != PICKUP_OK) {
    return PICKUP_ERR;
  }

  // Parse the received document
  if (parser_updates(ctx.buf, cb, data, last_activity_date) != 0) {
    free(ctx.buf);
    return PICKUP_ERR;
  }

  // Free buffer
  free(ctx.buf);

  return 0;
}

int pickup_match_clone(struct pickup_match *m, struct pickup_match **out) {
  struct pickup_match *cm;
  // Duplicate main struct
  cm = malloc(sizeof(struct pickup_match));
  if (cm == NULL) {
    return 1;
  }
  memcpy(cm, m, sizeof(struct pickup_match));
  // Images
  if (m->images_count > 0) {
    // Allocate images array
    cm->images = malloc(sizeof(struct pickup_image) * m->images_count);
    if (cm->images == NULL) {
      free(cm);
      return 1;
    }
    // Copy the array
    memcpy(&cm->images[0], &m->images[0], sizeof(struct pickup_image) *
        m->images_count);
  }
  // Messages
  if (m->messages_count > 0) {
    // Allocate messages array
    cm->messages = malloc(sizeof(struct pickup_message) * m->messages_count);
    if (cm->messages == NULL) {
      free(cm);
      free(&cm->images[0]);
      return 1;
    }
    // Copy the array
    memcpy(&cm->messages[0], &m->messages[0], sizeof(struct pickup_message) *
        m->messages_count);
  }
  *out = cm;
  return 0;
}

void pickup_match_free(struct pickup_match *m) {
  parser_match_free(m);
}

int pickup_match(const char *mid, struct pickup_updates_callbacks *cb,
    void *data) {
  CURL *curl;
  struct curl_slist *headers;
  struct context ctx;
  char url[0x100];

  if (pickup_is_auth() == 0) {
    return PICKUP_ERR_NO_ACCESS_TOKEN;
  }

  if (curl_prepare(&curl, &headers, &ctx) != 0) {
    return PICKUP_ERR;
  }

  sprintf(url, "%s%s/%s", API_HOST, API_MATCHES, mid);

  DEBUG("Match url dudes : %s\n", url);

  curl_easy_setopt(curl, CURLOPT_URL, url);

  if (curl_perform(curl, headers, &ctx) != 0) {
    return PICKUP_ERR;
  }

  // Check results
  if (ctx.error_code != PICKUP_OK) {
    return PICKUP_ERR;
  }

  // Parse the received document
  if (parser_prepare_match(ctx.buf, cb, data) != 0) {
    free(ctx.buf);
    return PICKUP_ERR;
  }

  // Free buffer
  free(ctx.buf);

  return 0;
}

int pickup_swipe(const char *pid, int like, int *remaining_likes,
    struct pickup_updates_callbacks *cb, void *data) {
  CURL *curl;
  struct curl_slist *headers;
  struct context ctx;
  char url[0x100], id_match[PICKUP_SIZE_ID];
  char *api;

  if (pickup_is_auth() == 0) {
    return PICKUP_ERR_NO_ACCESS_TOKEN;
  }

  if (curl_prepare(&curl, &headers, &ctx) != 0) {
    return PICKUP_ERR;
  }

  // Create the like or dislike url

  if (like) {
    api = API_LIKE;
  } else {
    api = API_UNLIKE;
  }

  sprintf(url, "%s%s/%s", API_HOST, api, pid);

  DEBUG("swipe url dudes : %s\n", url);

  curl_easy_setopt(curl, CURLOPT_URL, url);

  if (curl_perform(curl, headers, &ctx) != 0) {
    return PICKUP_ERR;
  }

  // Check results
  if (ctx.error_code != PICKUP_OK) {
    return PICKUP_ERR;
  }

  // Parse the received document
  memset(&id_match[0], 0, PICKUP_SIZE_ID);
  if (parser_swipe(ctx.buf, remaining_likes, &id_match[0]) != 0) {
    free(ctx.buf);
    return PICKUP_ERR;
  }

  // Free buffer
  free(ctx.buf);

  if (strlen(id_match) > 0) {
    // We have a match yeahh
    NOTE("We have a new match %s !\n", &id_match[0]);
    if (pickup_match(&id_match[0], cb, data) != 0) {
      ERROR("Error while getting new match info\n");
      return PICKUP_ERR;
    }
  }

  return 0;
}

int pickup_recs(struct pickup_recs_callbacks *cb, void *data) {
  CURL *curl;
  struct curl_slist *headers;
  struct context ctx;

  if (pickup_is_auth() == 0) {
    return PICKUP_ERR_NO_ACCESS_TOKEN;
  }

  if (curl_prepare(&curl, &headers, &ctx) != 0) {
    return PICKUP_ERR;
  }

  curl_easy_setopt(curl, CURLOPT_URL, API_HOST API_RECS);

  if (curl_perform(curl, headers, &ctx) != 0) {
    return PICKUP_ERR;
  }

  // Check results
  if (ctx.error_code != PICKUP_OK) {
    return PICKUP_ERR;
  }

  // Parse the received document
  if (parser_recs(ctx.buf, cb, data) != 0) {
    free(ctx.buf);
    return PICKUP_ERR;
  }

  // Free buffer
  free(ctx.buf);

  return 0;
}

int pickup_message(const char *mid, const char *message,
    struct pickup_message *msg) {
  CURL *curl;
  struct curl_slist *headers;
  struct context ctx;

  if (pickup_is_auth() == 0) {
    return PICKUP_ERR_NO_ACCESS_TOKEN;
  }

  if (curl_prepare(&curl, &headers, &ctx) != 0) {
    return PICKUP_ERR;
  }

  char url[0x100];
  sprintf(url, "%s%s/%s", API_HOST, API_MATCHES, mid);

  DEBUG("message url dudes : %s\n", url);

  char post_data[PICKUP_SIZE_MESSAGE];
  sprintf(post_data, "{\"message\":\"%s\"}", message);

  DEBUG("Data to post %s\n", post_data);

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

  if (curl_perform(curl, headers, &ctx) != 0) {
    return PICKUP_ERR;
  }

  // Check results
  if (ctx.error_code != PICKUP_OK) {
    return PICKUP_ERR;
  }

  if (parser_prepare_message(ctx.buf, msg) != 0) {
    ERROR("Error while parsing sent message\n");
    free(ctx.buf);
    return PICKUP_ERR;
  }

  // Free buffer
  free(ctx.buf);

  return 0;
}

void pickup_log_level(int l) {
  log_level(l);
}

void pickup_match_print(struct pickup_match *m) {
  int i, j;
  DEBUG("mid(%s)\n", m->mid);
  DEBUG("pid(%s)\n", m->pid);
  DEBUG("name(%s)\n", m->name);
  DEBUG("birth(%ld)\n", m->birth);
  DEBUG("messages_count(%ld)\n", m->messages_count);
  for (i = 0; i < m->messages_count; i++) {
    struct pickup_message *p = &m->messages[i];
    if (p->dir == PICKUP_MESSAGE_INPUT) {
      printf("she :\n");
    } else {
      printf("me :\n");
    }
    printf("%s\n", p->message);
  }
  DEBUG("images_count(%ld)\n", m->images_count);
  for (i = 0; i < m->images_count; i++) {
    struct pickup_image *p = &m->images[i];
    DEBUG("url(%s)\n", p->url);
    for (j = 0; j < 4; j++) {
      DEBUG("width(%d), height(%d), url(%s)\n", p->processed[j].width,
          p->processed[j].height, p->processed[j].url);
    }
  }
}
