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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

#include <pickup/pickup.h>
#include <oauth2webkit/oauth2webkit.h>

#include "http.h"
#include "api.h"
#include "io.h"
#include "db.h"
#include "log.h"

#define FB_TOKEN_NAME "pickup_fb_token"
#define PID_NAME "pickup_user_pid"
#define TOKEN_NAME "pickup_token"
#define LAST_ACTIVITY_DATE "last_activity_date"

static void usage(void) {
	fprintf(stderr, "Usage: xml updates\n");
	fprintf(stderr, "       xml matches list\n");
	fprintf(stderr, "       xml matches { print | images | gallery | update } MATCH\n");
	fprintf(stderr, "       xml matches message MATCH MESSAGE\n");
	fprintf(stderr, "       xml recs { list | scan }\n");
	fprintf(stderr, "       xml recs { print | like | unlike | images | gallery } REC\n");
	fprintf(stderr, "       xml user { credentials | auth | logout }\n");
  fprintf(stderr, "Options := { -h[help] | -v[erbose] | -q[uiet] |\n");
  fprintf(stderr, "             -d[ebug] | -list-possible-arguments }\n");
	exit(-1);
}

/**
 * Static flags and vars
 */
static int auth = 0;
static char access_token[0x100];
static char pid[PICKUP_SIZE_ID];

int matches(const char *cmd, const char *pattern);

int cb_block(char *mid, void *data) {
  NOTE("Got block by [%s] :'(\n", mid);
  if (db_delete_match(mid) != 0) {
    ERROR("Failed to delete the match\n");
    return -1;
  }
  return 0;
}

int cb_message(struct pickup_match *m, void *data) {
  int i;
  printf("New message for match %s\n", m->mid);
  for (i = 0; i < m->messages_count; i++) {
    if (db_update_message(&m->messages[i], m->mid) != 0) {
      ERROR("Failed to update insert a new message\n");
      return -1;
    }
  }
  return 0;
}

int cb_match(struct pickup_match *m, void *data) {
  printf("Update for match [%s]%s\n", m->pid, m->name);
  if (db_update_match(m) != 0) {
    ERROR("Failed to update the match\n");
    pickup_match_free(m);
    return -1;
  }
  return 0;
}

int cb_rec(struct pickup_match *m, void *data) {
  printf("New rec[%s]%s\n", m->pid, m->name);
  if (db_update_rec(m) != 0) {
    ERROR("Failed to update the rec\n");
    return -1;
  }
  return 0;
}

int user_auth(int *argc, char ***argv) {
  char fb_access_token[0x1000];
  int error_code;

  // We have to auth again
  oauth2_init(argc, argv);
  error_code = oauth2_get_access_token(FB_OAUTH2_URL, FB_OAUTH2_URL_CONFIRM,
      &fb_access_token[0]);

  if (error_code) {
    ERROR("Failed to get facebook access token : %d\n", error_code);
    return 1;
  }

  // Save the token
  str_write(FB_TOKEN_NAME, fb_access_token);

  error_code = pickup_auth(fb_access_token, access_token, pid);

  if (error_code) {
    ERROR("Failed to get access token : %d\n", error_code);
    return 1;
  }

  // Save the token
  if (str_write(TOKEN_NAME, access_token) != 0) {
    ERROR("Failed to write the access_token to %s\n", TOKEN_NAME);
  }

  // Save the token
  if (str_write(PID_NAME, pid) != 0) {
    ERROR("Failed to write the user pid to %s\n", PID_NAME);
  }

  // Set the tokens if any use if made after
  pickup_set_access_token(access_token, pid);

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

#define OPT_STR "vdqp:h"

static struct option long_options[] = {
  {"help", no_argument, 0, 'h'},
  {"verbose", no_argument, 0, 'v'},
  {"quiet", no_argument, 0, 'q'},
  {"debug", no_argument, 0, 'd'},
  {"list-possible-arguments", no_argument, 0, OPT_LIST_POSSIBLE_ARGUMENTS},
  {0, 0, 0, 0}
};

/**
 * Commands
 */

int cmd_updates(int argc, char **argv) {
  char last_activity_date[0x100];
  if (auth_check() != 0) {
    return -1;
  }
  struct pickup_updates_callbacks cbu = {
    cb_match,
    cb_message,
    cb_block
  };
  memset(last_activity_date, 0, 0x100);
  if (str_read(LAST_ACTIVITY_DATE, &last_activity_date[0], 0x100) != 0) {
    NOTE("Failed to read last activity date, set it to 0\n");
  } else {
    NOTE("Last activity was %s\n", &last_activity_date[0]);
  }
  if (pickup_updates(&cbu, NULL, &last_activity_date[0]) != 0) {
    ERROR("Failed to get the updates\n");
    return -1;
  }
  NOTE("Last activity %s\n", last_activity_date);
  if (str_write(LAST_ACTIVITY_DATE, &last_activity_date[0]) != 0) {
    ERROR("Failed to write last activity date\n");
    return -1;
  }
  return 0;
}

int cmd_message(int argc, char **argv) {
  struct pickup_match *m;
  struct pickup_message msg;
  char mid[PICKUP_SIZE_ID];
  if (auth_check() != 0) {
    return -1;
  }
  if (argc < 2) {
    ERROR("You have to specify the person to send a message !\n");
    return -1;
  }
  if (db_select_match(argv[0], &m) != 0) {
    ERROR("Failed to get the match\n");
    return -1;
  }
  strcpy(&mid[0], m->mid);
  if (pickup_message(&mid[0], argv[1], &msg) != 0) {
    ERROR("Failed to send a message to %s\n", argv[0]);
    pickup_match_free(m);
    return -1;
  }
  DEBUG("Adde the sent message to the database \n");
  if (db_update_message(&msg, m->mid) != 0) {
    pickup_match_free(m);
    return -1;
  }
  pickup_match_free(m);
  return 0;
}

int cmd_scan(int argc, char **argv) {
  if (auth_check() != 0) {
    return -1;
  }
  struct pickup_recs_callbacks cbr = {
    cb_rec,
  };
  return pickup_recs(&cbr, NULL);
}

int cmd_authenticate(int argc, char **argv) {
  DEBUG("Authenticate the user!\n");
  if (user_auth(&argc, &argv)) {
    ERROR("Failed to authenticate the user !\n");
    return 1;
  }
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
  file_unlink(FB_TOKEN_NAME, IO_PATH_CONFIG);
  file_unlink(TOKEN_NAME, IO_PATH_CONFIG);
  return 0;
}

void cb_match_list(struct pickup_match *m) {
  printf("%s %s\n", m->pid, m->name);
}

int cmd_list(int argc, char **argv) {
  db_select_matches(cb_match_list);
  return 0;
}

void cb_recs_list(struct pickup_match *m) {
  printf("%s %s\n", m->pid, m->name);
}

int cmd_list_recs(int argc, char **argv) {
  db_select_recs(cb_recs_list);
  return 0;
}

int cb_swipe_match(struct pickup_match *m, void *data) {
  int *new_match = data;
  *new_match = 1;
  return cb_match(m, NULL);
}

int cmd_unlike(int argc, char **argv) {
  int rl;
  int new_match;
  if (auth_check() != 0) {
    return -1;
  }
  if (argc < 1) {
    ERROR("please select a person\n");
    return -1;
  }
  struct pickup_updates_callbacks cbu = {
    cb_swipe_match,
  };
  if (pickup_swipe(argv[0], 0, &rl, &cbu, &new_match) != 0) {
    ERROR("Failed to unlike %s\n", argv[0]);
    return -1;
  }
  if (rl == -1) {
    NOTE("No likes consumed this time\n");
  } else {
    NOTE("Remaining likes %u, new_match %d\n", rl, new_match);
  }
  // We can remove the recommendation
  if (rl != 0) {
    DEBUG("We can drop the rec from the database\n");
    // We can remove the recommendation
    if (db_delete_person(argv[0]) != 0) {
      ERROR("Failed to delete the recommendation\n");
    }
  }
  return 0;
}

int cmd_like(int argc, char **argv) {
  int rl;
  int new_match = 0;
  if (auth_check() != 0) {
    return -1;
  }
  if (argc < 1) {
    ERROR("Please select a person\n");
    return -1;
  }
  struct pickup_updates_callbacks cbu = {
    cb_swipe_match,
  };
  if (pickup_swipe(argv[0], 1, &rl, &cbu, &new_match) != 0) {
    ERROR("Failed to like %s\n", argv[0]);
    return -1;
  }
  if (rl== -1) {
    NOTE("No likes consumed this time\n");
  } else {
    NOTE("Remaining likes %u, new_match %d\n", rl, new_match);
  }
  if (new_match == 0) {
    if (rl > 0) {
      // We can remove the recommendation
      if (db_delete_person(argv[0]) != 0) {
        ERROR("Failed to delete the recommendation\n");
      }
    }
  }
  return 0;
}

int cmd_print(int argc, char **argv) {
  struct pickup_match *m;
  if (argc < 1) {
    ERROR("Please select a person\n");
    return -1;
  }
  if (db_select_match(argv[0], &m) != 0) {
    ERROR("Error accessing the match in database\n");
    return -1;
  }
  pickup_match_print(m);
  pickup_match_free(m);
  return 0;
}

int cmd_print_rec(int argc, char **argv) {
  struct pickup_match *m;
  if (argc < 1) {
    ERROR("Please select a person\n");
    return -1;
  }
  if (db_select_rec(argv[0], &m) != 0) {
    ERROR("Error accessing the match in database\n");
    return -1;
  }
  pickup_match_print(m);
  pickup_match_free(m);
  return 0;
}

int image_download(struct pickup_image *img, int i, struct pickup_match *m) {
  size_t count;
  char *data;
  char filename[0x1000];
  // Create the filename
  // XXX .JPG
  sprintf(&filename[0], "%s_%s_%d.jpg", m->pid, img->id, i);
  NOTE("Downloading image %s\n", filename);
  if (http_download_file(img->url, &data, &count) != 0) {
    ERROR("Failed to download image %s\n", img->url);
    return -1;
  }
  DEBUG("Size of %s : %d bytes\n", img->url, count);
  // Write the image to disk
  if (file_write(filename, IO_PATH_CACHE_IMG, data, count) != 0) {
    ERROR("Failed to write image %s to disk\n", filename);
  }
  free(data);
  return 0;
}

int image_gallery(struct pickup_image *img, int i, struct pickup_match *m,
    char *args) {
  char path[0x1000];
  char filename[PICKUP_SIZE_ID + 10];
  // Create the filename
  // XXX .JPG
  sprintf(&filename[0], "%s_%s_%d.jpg", m->pid, m->images[i].id, i);
  DEBUG("Filename %s\n", filename);
  // We check if we have to download it
  if (file_exists(filename, IO_PATH_CACHE_IMG) != 0) {
    DEBUG("Image %s not downloaded yet : we do it\n", filename);
    if (image_download(img, i, m) != 0) {
      ERROR("Failed to download image %s\n", filename);
      return -1;
    }
  }
  if (path_resolve(filename, IO_PATH_CACHE_IMG, &path[0], 0x1000) != 0) {
    pickup_match_free(m);
    ERROR("Failed to resolve path for %s\n", filename);
    return -1;
  }
  // Add the name to the arguments !
  sprintf(args, "%s '%s'", args, path);
  return 0;
}

int cmd_match_gallery(int argc, char **argv) {
  struct pickup_match *m;
  int i;
  char args[0x1000], shell_command[0x1000];
  memset(args, 0, 0x1000);
  if (argc < 1) {
    ERROR("Please select a person\n");
    return -1;
  }
  if (db_select_match(argv[0], &m) != 0) {
    ERROR("Error accessing the match in database\n");
    return -1;
  }
  for (i = 0; i < m->images_count; i++) {
    image_gallery(&m->images[i], i, m, args);
  }
  DEBUG("Arguments %s\n", args);
  sprintf(shell_command, "feh %s", args);
  DEBUG("Shell command %s\n", shell_command);
  system(shell_command);
  pickup_match_free(m);
  return 0;
}

int cmd_match_update(int argc, char **argv) {
  struct pickup_match *m;
  if (argc < 1) {
    ERROR("Please select a person\n");
    return -1;
  }
  if (db_select_match(argv[0], &m) != 0) {
    ERROR("Error accessing the match in database\n");
    return -1;
  }
  struct pickup_updates_callbacks cbu = {
    cb_match,
    NULL,
    NULL,
  };
  if (pickup_match(m->mid, &cbu, NULL) != 0) {
    ERROR("Error while updating the match\n");
    return -1;
  }
  pickup_match_free(m);
  return 0;
}

int cmd_match_images(int argc, char **argv) {
  struct pickup_match *m;
  int i;
  if (argc < 1) {
    ERROR("Please select a person\n");
    return -1;
  }
  if (db_select_match(argv[0], &m) != 0) {
    ERROR("Error accessing the match in database\n");
    return -1;
  }
  for (i = 0; i < m->images_count; i++) {
    image_download(&m->images[i], i, m);
  }
  pickup_match_free(m);
  return 0;
}

int cmd_rec_gallery(int argc, char **argv) {
  struct pickup_match *m;
  int i;
  char args[0x1000], shell_command[0x1000];
  memset(args, 0, 0x1000);
  if (argc < 1) {
    ERROR("Please select a person\n");
    return -1;
  }
  if (db_select_rec(argv[0], &m) != 0) {
    ERROR("Error accessing the match in database\n");
    return -1;
  }
  for (i = 0; i < m->images_count; i++) {
    image_gallery(&m->images[i], i, m, args);
  }
  DEBUG("Arguments %s\n", args);
  sprintf(shell_command, "feh %s", args);
  DEBUG("Shell command %s\n", shell_command);
  system(shell_command);
  pickup_match_free(m);
  return 0;
}

int cmd_rec_images(int argc, char **argv) {
  struct pickup_match *m;
  int i;
  if (argc < 1) {
    ERROR("Please select a person\n");
    return -1;
  }
  if (db_select_rec(argv[0], &m) != 0) {
    ERROR("Error accessing the match in database\n");
    return -1;
  }
  for (i = 0; i < m->images_count; i++) {
    image_download(&m->images[i], i, m);
  }
  pickup_match_free(m);
  return 0;
}

int cmd_matches(int argc, char **argv) {
  if (argc < 1) {
    return cmd_list(argc - 1, argv + 1);
  } else if (matches(argv[0], "list") == 0) {
    return cmd_list(argc - 1, argv + 1);
  } else if (matches(argv[0], "message") == 0) {
    return cmd_message(argc - 1, argv + 1);
  } else if (matches(argv[0], "print") == 0) {
    return cmd_print(argc - 1, argv + 1);
  } else if (matches(argv[0], "gallery") == 0) {
    return cmd_match_gallery(argc - 1, argv + 1);
  } else if (matches(argv[0], "images") == 0) {
    return cmd_match_images(argc - 1, argv + 1);
  } else if (matches(argv[0], "update") == 0) {
    return cmd_match_update(argc - 1, argv + 1);
  }
  usage();
  return -1;
}

int cmd_recs(int argc, char **argv) {
  if (argc < 1) {
    return cmd_list_recs(argc - 1, argv + 1);
  } else if (matches(argv[0], "like") == 0) {
    return cmd_like(argc - 1, argv + 1);
  } else if (matches(argv[0], "unlike") == 0) {
    return cmd_unlike(argc - 1, argv + 1);
  } else if (matches(argv[0], "list") == 0) {
    return cmd_list_recs(argc - 1, argv + 1);
  } else if (matches(argv[0], "scan") == 0) {
    return cmd_scan(argc - 1, argv + 1);
  } else if (matches(argv[0], "print") == 0) {
    return cmd_print_rec(argc - 1, argv + 1);
  } else if (matches(argv[0], "gallery") == 0) {
    return cmd_rec_gallery(argc - 1, argv + 1);
  } else if (matches(argv[0], "images") == 0) {
    return cmd_rec_images(argc - 1, argv + 1);
  }
  usage();
  return -1;
}

int cmd_user(int argc, char **argv) {
  if (argc < 1) {
    return cmd_print_access_token(argc - 1, argv + 1);
  } else if (matches(argv[0], "credentials") == 0) {
    return cmd_print_access_token(argc - 1, argv + 1);
  } else if (matches(argv[0], "auth") == 0) {
    return cmd_authenticate(argc - 1, argv + 1);
  } else if (matches(argv[0], "logout") == 0) {
    return cmd_logout(argc - 1, argv + 1);
  }
  usage();
  return -1;
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
  {"updates", cmd_updates},
  {"matches", cmd_matches},
  {"recs", cmd_recs},
  {"user", cmd_user},
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
    c = getopt_long_only(argc, argv, OPT_STR, long_options, &option_index);

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
      case 'h':
        usage();
        break;
      case 'q':
        log_level(LOG_LEVEL_NONE);
        pickup_log_level(PICKUP_LOG_LEVEL_NONE);
        oauth2_log_level(PICKUP_LOG_LEVEL_NONE);
        break;
      case 'v':
        log_level(LOG_LEVEL_NOTE);
        pickup_log_level(PICKUP_LOG_LEVEL_NOTE);
        oauth2_log_level(PICKUP_LOG_LEVEL_NOTE);
        break;
      case 'd':
        log_level(LOG_LEVEL_DEBUG);
        pickup_log_level(PICKUP_LOG_LEVEL_DEBUG);
        oauth2_log_level(PICKUP_LOG_LEVEL_DEBUG);
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

  // Init pickup lib
  pickup_init();

  // Init DB connection
  db_init();

  // First ! We get the former access token in your pussy
  if (str_read(TOKEN_NAME, access_token, 0x100)) {
    NOTE("No access token found in dir ~/%s\n", IO_CONFIG_DIR);
  } else {
    NOTE("Access token found is %s\n", &access_token[0]);
    if (str_read(PID_NAME, pid, 0x100)) {
      NOTE("No access token found in dir ~/%s\n", IO_CONFIG_DIR);
    } else {
      NOTE("User pid found is %s\n", &pid[0]);
      // Set the access token and pid
      pickup_set_access_token(access_token, pid);
      auth = 1;
    }
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

  pickup_cleanup();
  db_cleanup();

  return 0;
}
