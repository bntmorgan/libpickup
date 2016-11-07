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

#ifndef __CINDER_H__
#define __CINDER_H__

#include <stdint.h>
#include <time.h>

enum cinder_log_level {
  CINDER_LOG_LEVEL_NONE,
  CINDER_LOG_LEVEL_ERROR,
  CINDER_LOG_LEVEL_NOTE,
  CINDER_LOG_LEVEL_DEBUG
};

enum cinder_error_code {
  CINDER_OK,
  CINDER_ERR_NO_FB_ACCESS_TOKEN,
  CINDER_ERR_NO_ACCESS_TOKEN,
  CINDER_ERR_NO_MEM
};

enum cinder_message_direction {
  CINDER_MESSAGE_INPUT,
  CINDER_MESSAGE_OUTPUT
};

#define CINDER_SIZE_MESSAGE 0x100
#define CINDER_SIZE_URL 0x100
#define CINDER_SIZE_NAME 0x40
#define CINDER_SIZE_ID 0x40
#define CINDER_SIZE_PROCESSED 4

struct cinder_message {
  char id[CINDER_SIZE_ID];
  enum cinder_message_direction dir;
  char message[CINDER_SIZE_MESSAGE];
  time_t date;
};

struct cinder_image_processed {
  char url[CINDER_SIZE_URL];
  short int width;
  short int height;
};

struct cinder_image {
  char id[CINDER_SIZE_ID];
  char url[CINDER_SIZE_URL];
  short int main;
  struct cinder_image_processed processed[CINDER_SIZE_PROCESSED];
};

struct cinder_match {
  char mid[CINDER_SIZE_ID];
  char pid[CINDER_SIZE_ID];
  char name[CINDER_SIZE_NAME];
  time_t date;
  time_t birth;
  unsigned int messages_count;
  struct cinder_message *messages;
  unsigned int images_count;
  struct cinder_image *images;
};

// Cinder updates callbacks
struct cinder_updates_callbacks {
  void (*match) (struct cinder_match *, void *);
  void (*message) (struct cinder_match *, void *);
  void (*block) (char *mid, void *);
};

// Cinder recs callbacks
struct cinder_recs_callbacks {
  void (*rec) (struct cinder_match *, void *);
};

void cinder_init(void);
void cinder_cleanup(void);
int cinder_auth(const char *fb_access_token, char *access_token,
    char *pid);
void cinder_set_access_token(const char *access_token, const char *pid);
int cinder_updates(struct cinder_updates_callbacks *cb, void *data,
    char *last_activity_date);
int cinder_recs(struct cinder_recs_callbacks *cb, void *data);
void cinder_match_free(struct cinder_match *m);
int cinder_swipe(const char *pid, int like, int *remaining_likes,
    struct cinder_updates_callbacks *cb, void *data);
int cinder_message(const char *mid, const char *message,
    struct cinder_message *msg);
void cinder_log_level(int l);
void cinder_match_print(struct cinder_match *m);
const char *cinder_get_access_token(void);
const char *cinder_get_pid(void);

#endif//__CINDER_H__
