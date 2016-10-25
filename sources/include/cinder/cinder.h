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

struct cinder_message {
  enum cinder_message_direction dir;
  char message[0x100];
};

struct cinder_picture_processed {
  char url[128];
  short int width;
  short int height;
};

struct cinder_picture {
  char url[128];
  char filename[64];
  short int main;
  struct cinder_picture_processed processed[4];
};

struct cinder_match {
  char name[64];
  char id[64];
  time_t birth;
  unsigned int messages_count;
  struct cinder_message *messages;
  unsigned int pictures_count;
  struct cinder_picture *pictures;
};

// Cinder updates callbacks
struct cinder_updates_callbacks {
  void (*match) (struct cinder_match *, void *);
};

// Cinder recs callbacks
struct cinder_recs_callbacks {
  void (*rec) (struct cinder_match *, void *);
};

void cinder_init(void);
void cinder_cleanup(void);
int cinder_authenticate(const char *fb_access_token, char *access_token);
void cinder_set_access_token(const char *access_token);
int cinder_updates(struct cinder_updates_callbacks *cb, void *data);
int cinder_recs(struct cinder_recs_callbacks *cb, void *data);
void cinder_match_free(struct cinder_match *m);
int parser_recs(const char *buf, struct cinder_recs_callbacks *cb, void *data);

#endif//__CINDER_H__
