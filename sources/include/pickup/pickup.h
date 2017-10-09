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

#ifndef __PICKUP_H__
#define __PICKUP_H__

#include <stdint.h>
#include <time.h>

enum pickup_log_level {
  PICKUP_LOG_LEVEL_NONE,
  PICKUP_LOG_LEVEL_ERROR,
  PICKUP_LOG_LEVEL_NOTE,
  PICKUP_LOG_LEVEL_DEBUG
};

enum parser_error_codes {
  PARSER_OK = 0,
};

enum pickup_error_code {
  PICKUP_OK,
  PICKUP_ERR_CB,
  PICKUP_ERR_PARSE_FAILED,
  PICKUP_ERR_NO_FB_ACCESS_TOKEN,
  PICKUP_ERR_NO_ACCESS_TOKEN,
  PICKUP_ERR_NO_MEM,
  PICKUP_ERR,
  PICKUP_ERR_UNAUTHORIZED,
  PICKUP_ERR_LAST,
};

enum pickup_message_direction {
  PICKUP_MESSAGE_INPUT,
  PICKUP_MESSAGE_OUTPUT
};

#define PICKUP_SIZE_MESSAGE 0x100
#define PICKUP_SIZE_URL 0x100
#define PICKUP_SIZE_NAME 0x40
#define PICKUP_SIZE_ID 0x40
#define PICKUP_SIZE_PROCESSED 4

struct pickup_message {
  char id[PICKUP_SIZE_ID];
  enum pickup_message_direction dir;
  char message[PICKUP_SIZE_MESSAGE];
  time_t date;
};

struct pickup_image_processed {
  char url[PICKUP_SIZE_URL];
  short int width;
  short int height;
};

struct pickup_image {
  char id[PICKUP_SIZE_ID];
  char url[PICKUP_SIZE_URL];
  short int main;
  struct pickup_image_processed processed[PICKUP_SIZE_PROCESSED];
};

struct pickup_match {
  char mid[PICKUP_SIZE_ID];
  char pid[PICKUP_SIZE_ID];
  char name[PICKUP_SIZE_NAME];
  time_t date;
  time_t birth;
  unsigned int messages_count;
  struct pickup_message *messages;
  unsigned int images_count;
  struct pickup_image *images;
};

// Pickup updates callbacks
struct pickup_updates_callbacks {
  int (*match) (struct pickup_match *, void *);
  int (*message) (struct pickup_match *, void *);
  int (*block) (char *mid, void *);
};

// Pickup recs callbacks
struct pickup_recs_callbacks {
  int (*rec) (struct pickup_match *, void *);
};

void pickup_init(void);
void pickup_cleanup(void);
int pickup_auth(const char *fb_access_token, char *access_token,
    char *pid);
void pickup_set_access_token(const char *access_token, const char *pid);
int pickup_updates(struct pickup_updates_callbacks *cb, void *data,
    char *last_activity_date);
int pickup_recs(struct pickup_recs_callbacks *cb, void *data);
int pickup_match_clone(struct pickup_match *m, struct pickup_match **out);
void pickup_match_free(struct pickup_match *m);
int pickup_swipe(const char *pid, int like, int *remaining_likes,
    struct pickup_updates_callbacks *cb, void *data);
int pickup_message(const char *mid, const char *message,
    struct pickup_message *msg);
void pickup_log_level(int l);
void pickup_match_print(struct pickup_match *m);
const char *pickup_get_access_token(void);
const char *pickup_get_pid(void);
int pickup_get_match(const char *mid, struct pickup_updates_callbacks *cb,
    void *data);
int pickup_get_person(const char *pid, struct pickup_updates_callbacks *cb,
    void *data);

#endif//__PICKUP_H__
