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
#include <getopt.h>

#include <cinder/cinder.h>
#include <oauth2webkit/oauth2webkit.h>

#include "api.h"
#include "io.h"
#include "log.h"

#define FB_TOKEN_NAME "cinder_fb_token"
#define TOKEN_NAME "cinder_token"

void cb_match(struct cinder_match *m, void *data) {
  cinder_match_print(m);
  cinder_match_free(m);
}

int user_auth(int *argc, char ***argv, char *access_token) {
  char fb_access_token[0x1000];
  int error_code;

  // We have to auth again
  oauth2_init(argc, argv);
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

  return 0;
}

#define AUTH_CHECK \
  if (auth == 0) { \
    ERROR("User must authenticate first !\n"); \
    break; \
  }

#define OPT_ACCESS_TOKEN 1
#define OPT_LOGOUT 2

static int auth = 0;

int main(int argc, char *argv[]) {
  char access_token[0x100];

  cinder_init();

  // First ! We get the former access token in your pussy
  if (str_read(TOKEN_NAME, access_token, 0x100)) {
    NOTE("No access token found in dir ~/%s\n", IO_CONFIG_DIR);
  } else {
    // Set the access token
    cinder_set_access_token(access_token);
    auth = 1;
  }

  // Parse the command line arguments
  int c;

  while (1) {
    static struct option long_options[] = {
      /* These options set a flag. */
//      {"verbose", no_argument,       &verbose_flag, 1},
//      {"brief",   no_argument,       &verbose_flag, 0},
      /* These options don’t set a flag.
         We distinguish them by their indices. */
      {"auth", no_argument, 0, 'a'},
      {"logout", no_argument, 0, OPT_LOGOUT},
      {"verbose", no_argument, 0, 'v'},
      {"quiet", no_argument, 0, 'q'},
      {"update", no_argument, 0, 'u'},
      {"debug", no_argument, 0, 'd'},
      {"print-access-token", no_argument, 0, OPT_ACCESS_TOKEN},
      {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "avduq",
        long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
      case 0:
        if (long_options[option_index].flag != 0)
          break;
        DEBUG("option %s", long_options[option_index].name);
        if (optarg)
          DEBUG_RAW(" with arg %s", optarg);
        DEBUG_RAW("\n");
        break;
      case 'a':
        DEBUG("Authenticate the user!\n");
        if (user_auth(&argc, &argv, access_token)) {
          ERROR("Failed to authenticate the user !\n");
          return 1;
        }
        cinder_set_access_token(access_token);
        break;
      case 'u':
        AUTH_CHECK;
        DEBUG("Update!\n");
        struct cinder_updates_callbacks cbu = {
          cb_match,
        };
        cinder_updates(&cbu, NULL);
        break;
      case 'q':
        log_level(LOG_LEVEL_NONE);
        cinder_log_level(CINDER_LOG_LEVEL_NONE);
        oauth2_log_level(CINDER_LOG_LEVEL_NONE);
        break;
      case 'v':
        log_level(LOG_LEVEL_NOTE);
        cinder_log_level(CINDER_LOG_LEVEL_NOTE);
        oauth2_log_level(CINDER_LOG_LEVEL_NOTE);
        break;
      case 'd':
        log_level(LOG_LEVEL_DEBUG);
        cinder_log_level(CINDER_LOG_LEVEL_DEBUG);
        oauth2_log_level(CINDER_LOG_LEVEL_DEBUG);
        break;
      case OPT_LOGOUT:
        AUTH_CHECK;
        DEBUG("Remove access_token file !\n");
        file_unlink(FB_TOKEN_NAME);
        file_unlink(TOKEN_NAME);
        break;
      case OPT_ACCESS_TOKEN:
        printf("%s\n", &access_token[0]);
        break;
      case '?':
        /* getopt_long already printed an error message. */
        break;
      default:
        ERROR("Error parsing the options\n");
        return 1;
    }
  }

  /* Print any remaining command line arguments (not options). */
  if (optind < argc) {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
        printf ("%s ", argv[optind++]);
      putchar ('\n');
    }


// Uncomment this example blocks !

//  struct cinder_recs_callbacks cbr = {
//    cb_match,
//  };
//
//  cinder_recs(&cbr, NULL);

//  unsigned int remaining_likes;
//  cinder_swipe("52b81a6c6c5685412c001881", 1, &remaining_likes);
//
//  printf("remaining likes %d\n", remaining_likes);

//  char message[] = "Salut, je suis cosmonaute et toi ?";
//  cinder_message("52b81a6c6c5685412c00188152f78ff9eb3d5fce16000a10", message);

  cinder_cleanup();

  return 0;
}
