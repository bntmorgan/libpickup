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

#include "cinder/cinder.h"
#include "oauth2webkit/oauth2webkit.h"
#include "api.h"
#include "io.h"

#define FB_TOKEN_NAME "cinder_fb_token"

int main(int argc, char *argv[]) {
  char access_token[0x1000];

  // First ! We get the former FB access token in your pussy
  if (str_read(FB_TOKEN_NAME, access_token, 0x1000)) {
    // We have to auth again
    oauth2_init(&argc, &argv);
    int error_code = oauth2_get_access_token(FB_OAUTH2_URL,
        FB_OAUTH2_URL_CONFIRM, &access_token[0]);

    if (error_code) {
      fprintf(stderr, "Failed to get facebook access token : %d\n", error_code);
      return 1;
    }

    // Save the token
    str_write(FB_TOKEN_NAME, access_token);
  }
  printf("Access_token dude %s\n", &access_token[0]);

  cinder_init();
  cinder_cleanup();

  return 0;
}
