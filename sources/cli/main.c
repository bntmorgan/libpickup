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
#include "db.h"
#include "log.h"

#define FB_TOKEN_NAME "cinder_fb_token"
#define TOKEN_NAME "cinder_token"

/**
 * Static flags and vars
 */
static int auth = 0;
static char access_token[0x100];

void cb_match(struct cinder_match *m, void *data) {
  // cinder_match_print(m);
  NOTE("Update for match [%s]%s\n", m->pid, m->name);
  db_update_match(m);
  cinder_match_free(m);
}

void cb_rec(struct cinder_match *m, void *data) {
  cinder_match_print(m);
  NOTE("Update for rec[%s]%s\n", m->pid, m->name);
  db_update_rec(m);
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

static inline int auth_check(void) {
  if (auth == 0) {
    ERROR("User must authenticate first !\n");
    return -1;
  }
  return 0;
}

/**
 * Options
 */

#define OPT_LIST_POSSIBLE_ARGUMENTS 1

#define OPT_STR "vdqp:"

static struct option long_options[] = {
  {"verbose", no_argument, 0, 'v'},
  {"quiet", no_argument, 0, 'q'},
  {"debug", no_argument, 0, 'd'},
  {"list-possible-arguments", no_argument, 0, OPT_LIST_POSSIBLE_ARGUMENTS},
  {0, 0, 0, 0}
};

/**
 * Commands
 */

int cmd_update(int argc, char **argv) {
  if (auth_check() != 0) {
    return -1;
  }
  struct cinder_updates_callbacks cbu = {
    cb_match,
  };
  return cinder_updates(&cbu, NULL);
}

int cmd_scan(int argc, char **argv) {
  if (auth_check() != 0) {
    return -1;
  }
  struct cinder_recs_callbacks cbr = {
    cb_rec,
  };
  return cinder_recs(&cbr, NULL);
}

int cmd_authenticate(int argc, char **argv) {
  DEBUG("Authenticate the user!\n");
  if (user_auth(&argc, &argv, access_token)) {
    ERROR("Failed to authenticate the user !\n");
    return 1;
  }
  cinder_set_access_token(access_token);
  return 0;
}

int cmd_print_access_token(int argc, char **argv) {
  if (auth_check() != 0) {
    return -1;
  }
  printf("%s\n", &access_token[0]);
  return 0;
}

int cmd_logout(int argc, char **argv) {
  if (auth_check() != 0) {
    return -1;
  }
  DEBUG("Remove access_token file !\n");
  file_unlink(FB_TOKEN_NAME);
  file_unlink(TOKEN_NAME);
  return 0;
}

void cb_match_list(struct cinder_match *m) {
  printf("[%s]%s\n", m->pid, m->name);
}

int cmd_list(int argc, char **argv) {
  db_select_matches_persons(cb_match_list);
  return 0;
}

int cmd_unlike(int argc, char **argv) {
  unsigned int rl;
  if (auth_check() != 0) {
    return -1;
  }
  if (argc < 1) {
    ERROR("please select a person\n");
    return -1;
  }
  if (cinder_swipe(argv[0], 0, &rl) != 0) {
    ERROR("Failed to unlike %s\n", argv[0]);
    return -1;
  }
  NOTE("Remaining likes %u\n", rl);
  // We can remove the recommendation
  if (db_delete_person(argv[0]) != 0) {
    ERROR("Failed to delete the recommendation\n");
  }
  return 0;
}

int cmd_like(int argc, char **argv) {
  unsigned int rl;
  if (auth_check() != 0) {
    return -1;
  }
  if (argc < 1) {
    ERROR("please select a person\n");
    return -1;
  }
  if (cinder_swipe(argv[0], 1, &rl) != 0) {
    ERROR("Failed to like %s\n", argv[0]);
    return -1;
  }
  NOTE("Remaining likes %u\n", rl);
  // We can remove the recommendation
  if (db_delete_person(argv[0]) != 0) {
    ERROR("Failed to delete the recommendation\n");
  }
  return 0;
}

/**
 * Command management
 */

// Commands and callbacks
// thanks to iproute2-4.8.0

int matches(const char *cmd, const char *pattern) {
  int len = strlen(cmd);
  if (len > strlen(pattern)) {
    return -1;
  }
  return memcmp(pattern, cmd, len);
}

static const struct cmd {
  const char *cmd;
  int (*func)(int argc, char **argv);
} cmds[] = {
  {"update", cmd_update},
  {"scan", cmd_scan},
  {"authenticate", cmd_authenticate},
  {"print-access-token", cmd_print_access_token},
  {"list", cmd_list},
  {"like", cmd_like},
  {"unlike", cmd_unlike},
  {"logout", cmd_logout},
  { 0 }
};

static int do_cmd(const char *argv0, int argc, char **argv) {
  const struct cmd *c;

  for (c = cmds; c->cmd; ++c) {
    if (matches(argv0, c->cmd) == 0) {
      DEBUG("Match for %s\n", c->cmd);
      return -(c->func(argc-1, argv+1));
    }
  }

  ERROR("Object \"%s\" is unknown, try \"xml --help\".\n", argv0);
  return -1;
}

int main(int argc, char *argv[]) {
  int c;
  int option_index = 0;

  /**
   * First configuration options
   */
  while (1) {
    c = getopt_long (argc, argv, OPT_STR, long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
      case OPT_LIST_POSSIBLE_ARGUMENTS: {
        const struct cmd *c = &cmds[0];
        int i = 0;
        while (c->cmd != NULL) {
          printf("%s\n", c->cmd);
          i++;
          c = &cmds[i];
        }
        return 0;
      }
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
      case '?':
        /* getopt_long already printed an error message. */
        break;
      default:
        // The second stage getopt will handle it
        NOTE("%d optind\n", optind);
        break;
    }
  }

  /**
   * Then initialize the libraries
   */

  // Init cinder lib
  cinder_init();

  // Init DB connection
  db_init();

  // First ! We get the former access token in your pussy
  if (str_read(TOKEN_NAME, access_token, 0x100)) {
    NOTE("No access token found in dir ~/%s\n", IO_CONFIG_DIR);
  } else {
    // Set the access token
    cinder_set_access_token(access_token);
    auth = 1;
  }

  /**
   * Finally, execute commands
   */

  if (optind < argc) {
    return do_cmd(argv[optind], argc-optind, argv+optind);
  }

  /**
   * Then clean the libraries
   */

  cinder_cleanup();
  db_cleanup();

  return 0;
}
