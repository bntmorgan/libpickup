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
#include <string.h>
#include <time.h>

#include <pickup/pickup.h>
#include <oauth2webkit/oauth2webkit.h>

#include "api.h"
#include "io.h"

#define PID_NAME "pickup_user_pid"
#define FB_TOKEN_NAME "pickup_fb_token"
#define TOKEN_NAME "pickup_token"

void cb_match(struct pickup_match *m, void *data) {
  pickup_match_print(m);
  pickup_match_free(m);
}

int main(int argc, char *argv[]) {

  char fb_access_token[0x1000];
  char access_token[0x100];
  char pid[PICKUP_SIZE_ID];
  int error_code;

  pickup_log_level(PICKUP_LOG_LEVEL_DEBUG);
  pickup_init();

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

    error_code = pickup_auth(fb_access_token, access_token, pid);

    if (error_code) {
      fprintf(stderr, "Failed to get access token : %d\n", error_code);
      return 1;
    }

    // Save the token
    str_write(TOKEN_NAME, access_token);

    // Save the user pid
    str_write(PID_NAME, pid);
  }

  if (str_read(PID_NAME, access_token, 0x100)) {
    fprintf(stderr, "Failed to read user PID file\n");
    return -1;
  }

  printf("Access_token dude %s\n", &access_token[0]);
  pickup_set_access_token(access_token, pid);

// Uncomment this example blocks !

//  struct pickup_updates_callbacks cbu = {
//    cb_match,
//  };
//
//  pickup_updates(&cbu, NULL);

//  struct pickup_recs_callbacks cbr = {
//    cb_match,
//  };
//
//  pickup_recs(&cbr, NULL);

//  unsigned int remaining_likes;
//  pickup_swipe("52b81a6c6c5685412c001881", 1, &remaining_likes);
//
//  printf("remaining likes %d\n", remaining_likes);

//  char message[] = "Salut, je suis cosmonaute et toi ?";
//  pickup_message("52b81a6c6c5685412c00188152f78ff9eb3d5fce16000a10", message);

  pickup_cleanup();

  return 0;
}
