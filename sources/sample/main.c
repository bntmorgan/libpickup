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
#include <string.h>
#include <time.h>

#include <cinder/cinder.h>
#include <oauth2webkit/oauth2webkit.h>

#include "api.h"
#include "io.h"

#define FB_TOKEN_NAME "cinder_fb_token"
#define TOKEN_NAME "cinder_token"

void cb_match(struct cinder_match *m, void *data) {
  int i, j;
  printf("id(%s)\n", m->id);
  printf("name(%s)\n", m->name);
  printf("birth(%ld)\n", m->birth);
  for (i = 0; i < m->pictures_count; i++) {
    struct cinder_picture *p = &m->pictures[i];
    printf("url(%s)\n", p->url);
    for (j = 0; j < 4; j++) {
      struct cinder_picture_processed *pr = &p->processed[j];
      printf("width(%d), height(%d), url(%s)\n", pr->width, pr->height,
          pr->url);
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
  cinder_match_free(m);
}

int main(int argc, char *argv[]) {

  char fb_access_token[0x1000];
  char access_token[0x100];
  int error_code;

  cinder_init();

  // First ! We get the former access token in your pussy
  if (str_read(TOKEN_NAME, access_token, 0x100)) {
    // We have to auth again
    oauth2_init(&argc, &argv);
    error_code = oauth2_get_access_token(FB_OAUTH2_URL, FB_OAUTH2_URL_CONFIRM,
        &fb_access_token[0]);

    if (error_code) {
      fprintf(stderr, "Failed to get facebook access token : %d\n", error_code);
      return 1;
    }

    // Save the token
    str_write(FB_TOKEN_NAME, fb_access_token);

    error_code = cinder_authenticate(fb_access_token, access_token);

    if (error_code) {
      fprintf(stderr, "Failed to get access token : %d\n", error_code);
      return 1;
    }

    // Save the token
    str_write(TOKEN_NAME, access_token);

  }

  printf("Access_token dude %s\n", &access_token[0]);
  cinder_set_access_token(access_token);

// Uncomment this example blocks !

//  struct cinder_updates_callbacks cbu = {
//    cb_match,
//  };
//
//  cinder_updates(&cbu, NULL);

//  struct cinder_recs_callbacks cbr = {
//    cb_match,
//  };
//
//  cinder_recs(&cbr, NULL);

//  unsigned int remaining_likes;
//  cinder_swipe("52b81a6c6c5685412c001881", 1, &remaining_likes);
//
//  printf("remaining likes %d\n", remaining_likes);

  cinder_cleanup();

  return 0;
}
