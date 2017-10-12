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

#include <string.h>
#include <stdlib.h>

#include <pickup/pickup.h>
#include <oauth2webkit/oauth2webkit.h>

#include "api.h"
#include "model.h"
#include "db.h"
#include "log.h"
#include "http.h"
#include "io.h"
#include "message.h"
#include "controller.h"
#include "note.h"
#include "worker.h"

#define FB_TOKEN_NAME "pickup_fb_token"
#define PID_NAME "pickup_user_pid"
#define TOKEN_NAME "pickup_token"
#define LAST_ACTIVITY_DATE "last_activity_date"

/**
 * Static flags and vars
 */

void controller_cleanup(void) {
  model_cleanup();
  pickup_cleanup();
}

void user_connect(void) {
  g_object_set(user, "auth", 1, NULL);
}

void user_disconnect(void) {
  g_object_set(user, "auth", 0, NULL);
}

void controller_init(void) {
  char access_token[0x100], *at;
  char pid[PICKUP_SIZE_ID], *p;
  pickup_init();
  model_init();
  model_populate();
  // First ! We get the former access token in your pussy
  if (str_read(TOKEN_NAME, &access_token[0], 0x100)) {
    NOTE("No access token found in dir ~/%s\n", IO_CONFIG_DIR);
  } else {
    NOTE("Access token found is %s\n", &access_token[0]);
    g_object_set(user, "access-token", &access_token[0], NULL);
    if (str_read(PID_NAME, &pid[0], 0x100)) {
      NOTE("No pid found in dir ~/%s\n", IO_CONFIG_DIR);
    } else {
      NOTE("User pid found is %s\n", &pid[0]);
      user_connect();
      g_object_get(user, "access-token", &at, "pid", &p, NULL);
      // Set the access token and pid
      pickup_set_access_token(at, p);
    }
  }
}

#define IMG_FILENAME_MAX PICKUP_SIZE_ID * 2 + 10

int image_after(void *data) {
  char path[MAX_FILE_PATH];
  char *filename = data;
  DEBUG("Image %s downloaded : we set it\n", filename);
  // No worker
  if (path_resolve(filename, IO_PATH_CACHE_IMG, &path[0],
        MAX_FILE_PATH) != 0) {
    ERROR("Failed to resolve path for %s\n", filename);
    free(filename);
    return -1;
  }
  g_object_set(selected, "image", &path[0], NULL);
  free(filename);
  return 0;
}

struct image_download_param {
  struct pickup_image img;
  char filename[IMG_FILENAME_MAX];
};

int image_download_worker(void *data) {
  struct image_download_param *p = data;
  size_t count;
  char *buf;
  NOTE("Downloading image %s\n", p->filename);
  if (http_download_file(p->img.url, &buf, &count) != 0) {
    ERROR_NOTE_WORKER("Failed to download image %s\n", p->img.url);
    free(p);
    return -1;
  }
  DEBUG("Size of %s : %d bytes\n", p->img.url, count);
  // Write the image to disk
  if (file_write(p->filename, IO_PATH_CACHE_IMG, buf, count) != 0) {
    ERROR_NOTE_WORKER("Failed to write image %s to disk\n", p->filename);
    free(buf);
    free(p);
    return -1;
  }
  // We set image path in with an idle after
  worker_idle_add(image_after, strdup(p->filename));
  free(buf);
  free(p);
  return 0;
}

int image_gallery(struct pickup_image *img, int i, char *pid) {
  char filename[IMG_FILENAME_MAX];
  // Create the filename
  // XXX .JPG
  snprintf(&filename[0], IMG_FILENAME_MAX, "%s_%s_%d.jpg", pid, img->id, i);
  DEBUG("Filename %s\n", filename);
  // We check if we have to download it
  if (file_exists(filename, IO_PATH_CACHE_IMG) != 0) {
    struct image_download_param *p =
      malloc(sizeof(struct image_download_param));
    if (p == NULL) {
      return -1;
    }
    memcpy(&p->img, img, sizeof(struct pickup_image));
    strcpy(&p->filename[0], &filename[0]);
    // Go to worker !
    DEBUG("Image %s not downloaded yet : we do it\n", filename);
    worker_run("image_download_worker", image_download_worker, p);
  } else {
    char path[MAX_FILE_PATH];
    DEBUG("Image %s downloaded : we set it\n", filename);
    // No worker
    if (path_resolve(filename, IO_PATH_CACHE_IMG, &path[0],
          MAX_FILE_PATH) != 0) {
      ERROR("Failed to resolve path for %s\n", filename);
      return -1;
    }
    g_object_set(selected, "image", &path[0], NULL);
  }
  return 0;
}

void controller_clear_match(void) {
  DEBUG("Clear match window\n");
  g_object_set(selected, "pid", "", "name", "", "birth", 0, "images", 0,
      "images-count", 0, "image-index", 0, "image", "", "set", 0, NULL);
  // XXX "set" has to be set before "match" attribute.... ???
  g_object_set(selected, "match", 0, "image-progress", 0, "index", 0, "lock", 0,
      NULL);
}

void set_match(struct pickup_match *m, int match, unsigned int index) {
  struct pickup_image *images;
  if (image_gallery(&m->images[0], 0, m->pid)) {
    ERROR("Error while getting image to display\n");
  }
  // Duplicate image array
  g_object_get(selected, "images", &images, NULL);
  if (images) {
    free(images);
  }
  images = malloc(sizeof(struct pickup_image) * m->images_count);
  memcpy(images, m->images, sizeof(struct pickup_image) * m->images_count);
  DEBUG("IS MATCH LORD %d, %d\n", match, TRUE);
  // Free existing message data
  // and Clear the list
  g_list_store_remove_all(messages);
  // Populate message if any
  if (m->messages_count) {
    int i;
    // Populate
    for (i = 0; i < m->messages_count; i++) {
      Message *msg;
      msg = g_object_new(message_get_type(), "id", m->messages[i].id,
          "dir", m->messages[i].dir, "date", m->messages[i].date,
          "message", m->messages[i].message, NULL);
      g_list_store_append(messages, msg);
    }
  }
  // Set match attributes
  float progress = (float) 1 / m->images_count;
  g_object_set(selected, "mid", m->mid, "pid", m->pid, "name", m->name, "birth",
      m->birth, "images", &images[0], "images-count", m->images_count,
      "image-index", 0, "match", match, "image-progress", progress, "index",
      index, "set", 1, NULL);
}

void controller_image_skip(int skip) {
  gint index, count, set;
  gchar *pid;
  gfloat progress;
  struct pickup_image *images;
  DEBUG("Skipping %d images\n", skip);
  g_object_get(selected, "image-index", &index, "images-count", &count,
      "images", &images, "pid", &pid, "set", &set, NULL);
  if (set == 0) {
    DEBUG("Match is not set\n");
  } else {
    DEBUG("Current image index %d / %d, pid[%s], images[%p]\n", index, count, pid,
        images);
    index = (index + skip) % count;
    index = (index >= 0) ? index : index + count;
    DEBUG("New image index %d / %d\n", index, count);
    if (image_gallery(&images[index], index, pid)) {
      ERROR("Error while getting image to display\n");
    }
    // Finally set the path
    progress = (gfloat) (index + 1)/ count;
    DEBUG("Progress %f\n", progress);
    g_object_set(selected, "image-index", index, NULL);
    g_object_set(selected, "image-progress", progress, NULL);
  }
}

void controller_set_match(const char *pid, unsigned int index) {
  struct pickup_match *m;
  if (db_select_match(pid, &m)) {
    ERROR_NOTE("Failed to match %s from db\n", pid);
    return;
  }
  set_match(m, 1, index);
  pickup_match_free(m);
}

void controller_set_rec(const char *pid, unsigned int index) {
  struct pickup_match *m;
  if (db_select_rec(pid, &m)) {
    ERROR("Failed to rec %s from db\n", pid);
    return;
  }
  set_match(m, 0, index);
  pickup_match_free(m);
}

int cb_match_idle(void *data) {
  struct pickup_match *m = data;
  MatchList *obj;
  obj = g_object_new(match_list_get_type(), "mid", m->mid, "pid", m->pid,
      "name", m->name, "date", m->date, "birth", m->birth, NULL);
  g_list_store_append(matches, obj);
  return 0;
}

int cb_match(struct pickup_match *m, void *data) {
  struct pickup_match *cm;
  printf("Update for match [%s]%s\n", m->pid, m->name);
  if (db_insert_match(m) != 0) {
    ERROR("Failed to update the match\n");
    return -1;
  }
  pickup_match_print(m);
  if (pickup_match_clone(m, &cm)) {
    ERROR("Failed to clone match\n");
    return -1;
  }
  worker_idle_add(cb_match_idle, cm);
  return 0;
}

struct swipe_rec_after_param {
  int ret;
  int like;
  int rl;
  int is_match;
  struct pickup_match *sm;
  char *pid;
};

int swipe_rec_after(void *data) {
  struct swipe_rec_after_param *p = data;
  unsigned int index;
  DEBUG("Remaining likes %d, like ? %d\n", p->rl, p->like);
  if (p->rl == 0) {
    ERROR_NOTE("Failed to swipe %s, no more swipes, try later...\n", p->pid);
  } else {
    // We can remove the match from rec glist store
    g_object_get(selected, "index", &index, NULL);
    DEBUG("Index to remove %d\n", index);
    g_list_store_remove(recs, index);
    // If it is a match : we insert it into the matches glist store
    if (p->is_match) {
      // Calls the cb_match_idle to add the match in g_list_store
      cb_match_idle(p->sm);
    }
  }
  if (p->is_match) {
    free(p->sm);
  }
  free(p);
  return 0;
}

int cb_swipe_match(struct pickup_match *m, void *data) {
  struct pickup_match **cm = data;
  NOTE("Swipe person has matched ! [%s] %s, clone it\n", m->pid, m->name);
  int ret = pickup_match_clone(m, cm);
  if (ret != 0) {
    ERROR_NOTE_WORKER("Failed to clone the swiped match %s, %d\n", m->pid, ret);
    return 1;
  }
  pickup_match_print(m);
  return 0;
}

struct swipe_rec_worker_param {
  int like;
  char *pid;
};

int user_disconnect_after(void *data) {
  user_disconnect();
  return 0;
}

#define WORKER_PICKUP_HANDLE_CODE(ret) { \
  if (ret == PICKUP_ERR_UNAUTHORIZED) { \
    ERROR_NOTE_WORKER("User has credentials are valid no more\n"); \
    worker_idle_add(user_disconnect_after, NULL); \
  } else if (ret == PICKUP_ERR_NETWORK) { \
    ERROR_NOTE_WORKER("Pikcup lib experienced network issues\n"); \
  } \
}

int swipe_rec_worker(void *data) {
  int rl = 0;
  int ret;
  int is_match = 0;
  struct pickup_match *sm = NULL;
  struct swipe_rec_worker_param *p = data;
  struct pickup_updates_callbacks cbu = {
    cb_swipe_match,
  };
  ret = pickup_swipe(p->pid, p->like, &rl, &cbu, &sm);
  WORKER_PICKUP_HANDLE_CODE(ret);
  if (ret != 0) {
    ERROR_NOTE_WORKER("Failed to dislike %s\n", p->pid);
    free(p);
    return -1;
  }
  if (sm != NULL) {
    DEBUG("is is not null, swiped rec is a match\n");
    is_match = 1;
  }
  // Handle database operations
  DEBUG("Remaining likes %d, like ? %d\n", rl, p->like);
  if (rl == 0) {
    NOTE("Failed to swipe %s, no more swipes\n", p->pid);
  } else {
    // We can remove the match from the rec table in db
    if (db_delete_person(p->pid) != 0) {
      ERROR("Failed to delete the recommendation\n");
      if (is_match) {
        free(sm);
      }
      free(p);
      return -1;
    }
    if (is_match) {
      // If it is a match : we insert it into the db
      if (db_insert_match(sm) != 0) {
        ERROR("Failed to insert a match\n");
        free(sm);
        free(p);
        return -1;
      }
    }
  }
  // Prepare after for gui operations
  struct swipe_rec_after_param *pa = malloc(sizeof(struct
        swipe_rec_after_param));
  pa->ret = ret;
  pa->rl = rl;
  pa->like = p->like;
  pa->pid = p->pid;
  pa->is_match = is_match;
  pa->sm = sm;
  free(p);
  worker_idle_add(swipe_rec_after, pa);
  return 0;
}

void controller_swipe_rec(int like) {
  struct swipe_rec_worker_param *p = malloc(sizeof(struct
        swipe_rec_worker_param));
  g_object_get(selected, "pid", &p->pid, NULL);
  p->like = like;
  DEBUG("Swiping rec[%s] like %d\n", p->pid, like);
  worker_run("swipe_rec_worker", swipe_rec_worker, p);
}

int cb_rec_idle(void *data) {
  MatchList *obj;
  struct pickup_match *m = data;
  obj = g_object_new(match_list_get_type(), "pid", m->pid,
      "name", m->name, "date", m->date, "birth", m->birth, NULL);
  g_list_store_append(recs, obj);
  return 0;
}

int cb_rec(struct pickup_match *m, void *data) {
  struct pickup_match *cm;
  DEBUG("New rec[%s]%s\n", m->pid, m->name);
  if (db_update_rec(m) != 0) {
    ERROR("Failed to update the rec\n");
    return -1;
  }
  pickup_match_print(m);
  if (pickup_match_clone(m, &cm)) {
    ERROR("Failed to clone match\n");
    return -1;
  }
  worker_idle_add(cb_rec_idle, cm);
  return 0;
}

int recs_scan_worker(void *data) {
  int ret;
  struct pickup_recs_callbacks cbr = {
    cb_rec,
  };

  ret = pickup_recs(&cbr, NULL);
  WORKER_PICKUP_HANDLE_CODE(ret);
  if (ret != 0) {
    ERROR_NOTE_WORKER("Failed to scan new recs\n");
    return -1;
  }
  return 0;
}

int controller_recs_scan(void) {
  DEBUG("Scanning new recs\n");
  worker_run("recs_scan_worker", recs_scan_worker, NULL);
  return 0;
}

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

int updates_worker(void *data) {
  int ret;
  char last_activity_date[0x100];
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
  ret = pickup_updates(&cbu, NULL, &last_activity_date[0]);
  WORKER_PICKUP_HANDLE_CODE(ret);
  if (ret != 0) {
    ERROR_NOTE_WORKER("Failed to get the updates\n");
    return -1;
  }
  NOTE("Last activity %s\n", last_activity_date);
  if (str_write(LAST_ACTIVITY_DATE, &last_activity_date[0]) != 0) {
    ERROR_NOTE_WORKER("Failed to write last activity date\n");
    return -1;
  }
  return 0;
}

int controller_updates(void) {
  DEBUG("Scanning new updates\n");
  worker_run("updates_worker", updates_worker, NULL);
  return 0;
}

struct message_after_param {
  char *mid;
  struct pickup_message msg;
};

int message_after(void *data) {
  struct message_after_param *p = data;
  Message *message;
  // Add the message to the model
  message = g_object_new(message_get_type(), "id", p->msg.id,
      "dir", p->msg.dir, "date", p->msg.date,
      "message", p->msg.message, NULL);
  g_list_store_append(messages, message);
  free(p);
  return 0;
}

struct message_worker_param {
  char text[PICKUP_SIZE_MESSAGE];
  char *mid;
};

int message_worker(void *data) {
  struct message_worker_param *p = data;
  struct message_after_param *pa;
  struct pickup_message msg;
  int ret;
  DEBUG("Send message %s to %s\n", p->text, p->mid);
  ret = pickup_message(p->mid, p->text, &msg);
  WORKER_PICKUP_HANDLE_CODE(ret);
  if (ret != 0) {
    ERROR_NOTE_WORKER("Failed to send a message to %s\n", p->mid);
    free(p);
    return -1;
  }
  pa = malloc(sizeof(struct pickup_message));
  if (pa == NULL) {
    free(p);
    return -1;
  }
  memcpy(&pa->msg, &msg, sizeof(struct pickup_message));
  DEBUG("Add the sent message to the database \n");
  if (db_update_message(&msg, p->mid) != 0) {
    free(p);
    return -1;
  }
  free(p);
  worker_idle_add(message_after, pa);
  return 0;
}

int controller_message(char *text) {
  struct message_worker_param *p = malloc(sizeof(struct message_worker_param));
  if (p == NULL) {
    return -1;
  }
  strcpy(&p->text[0], text);
  g_object_get(selected, "mid", &p->mid, NULL);
  worker_run("message_worker", message_worker, p);
  return 0;
}

void controller_lock(int lock) {
  DEBUG("Lock %d\n", lock);
  g_object_set(selected, "lock", lock, NULL);
}

void note_add(int type, char *msg) {
  Note *obj;

  obj = g_object_new(note_get_type(), "type", type, "message", msg, NULL);
  g_list_store_append(notes, obj);
}

struct note_add_idle_param {
  char *msg;
  int type;
};

int note_add_idle(void *data) {
  struct note_add_idle_param *p = data;
  note_add(p->type, p->msg);
  free(p->msg);
  free(p);
  return 0;
}

void controller_note_add_idle(int type, char *format, ...) {
  // XXX size !
  struct note_add_idle_param *p = malloc(sizeof(struct note_add_idle_param));
  char buf[0x100];
  va_list ap;

  va_start(ap, format);
  vsprintf(&buf[0], format, ap);
  va_end(ap);

  p->type = type;
  p->msg = strdup(&buf[0]);
  worker_idle_add(note_add_idle, p);
}

void controller_note_add(int type, char *format, ...) {
  // XXX size !
  char buf[0x100];
  va_list ap;

  va_start(ap, format);
  vsprintf(&buf[0], format, ap);
  va_end(ap);

  note_add(type, &buf[0]);
}

void controller_note_closed(Note *note) {
  Note *cn;
  unsigned int i, n = g_list_model_get_n_items(G_LIST_MODEL(notes));
  for (i = 0; i < n; i++) {
    cn =  g_list_model_get_item(G_LIST_MODEL(notes), i);
    if (note == cn) {
      DEBUG("Found the note, remove it\n");
      g_list_store_remove(notes, i);
      break;
    }
  }
}

int cb_update_match_idle(void *data) {
  struct pickup_match *m = data;
  int index;
  g_object_get(selected, "index", &index, NULL);
  set_match(m, 1, index);
  pickup_match_free(m);
  return 0;
}

int cb_update_match(struct pickup_match *m, void *data) {
  struct pickup_match *cm;
  printf("Update for match [%s]%s\n", m->pid, m->name);
  if (db_update_match(m) != 0) {
    ERROR("Failed to update the match\n");
    return -1;
  }
  pickup_match_print(m);
  if (pickup_match_clone(m, &cm)) {
    ERROR("Failed to clone match\n");
    return -1;
  }
  worker_idle_add(cb_update_match_idle, cm);
  return 0;
}

int match_update_worker(void *data) {
  char *mid = data;
  int ret;
  struct pickup_updates_callbacks cb = {
    cb_update_match,
  };
  ret = pickup_get_match(mid, &cb, NULL);
  WORKER_PICKUP_HANDLE_CODE(ret);
  if (ret != 0) {
    ERROR_NOTE_WORKER("Failed to update match %s\n", mid);
    return -1;
  }
  return 0;
}

void controller_match_update(void) {
  char *mid;
  g_object_get(selected, "mid", &mid, NULL);
  DEBUG("Update match mid(%s)\n", mid);
  worker_run("match_update_worker", match_update_worker, mid);
}

struct auth_after_param {
  char fb_access_token[0x100];
  char access_token[0x100];
  char pid[0x100];
};

int auth_after(void *data) {
  struct auth_after_param *p = data;
  char *access_token, *pid;
  DEBUG("Auth after\n");
  g_object_set(user, "access-token", p->access_token, "fb-access-token",
      p->fb_access_token, "pid", p->pid, NULL);
  user_connect();
  g_object_get(user, "access-token", &access_token, "pid", &pid, NULL);
  // Set the tokens if any use if made after
  pickup_set_access_token(access_token, pid);
  free(p);
  return 0;
}

int auth_worker(void *data) {
  char *fb_access_token = data;
  char access_token[0x100];
  char pid[0x100];
  struct auth_after_param *pa;
  int error_code;

  // Save fb access token
  if (str_write(FB_TOKEN_NAME, fb_access_token) != 0) {
    ERROR_NOTE_WORKER("Failed to write the access_token to %s\n",
        FB_TOKEN_NAME);
    free(fb_access_token);
    return 1;
  }

  error_code = pickup_auth(fb_access_token, access_token, pid);

  if (error_code) {
    ERROR_NOTE_WORKER("Failed to get access token : %d\n", error_code);
    free(fb_access_token);
    return 1;
  }

  // Save the token
  if (str_write(TOKEN_NAME, access_token) != 0) {
    ERROR_NOTE_WORKER("Failed to write the access_token to %s\n", TOKEN_NAME);
    free(fb_access_token);
    return 1;
  }

  // Save the token
  if (str_write(PID_NAME, pid) != 0) {
    ERROR_NOTE_WORKER("Failed to write the user pid to %s\n", PID_NAME);
    free(fb_access_token);
    return 1;
  }

  pa = malloc(sizeof(struct auth_after_param));
  if (pa == NULL) {
    ERROR_NOTE_WORKER("Failed to allocate memory to commit the "
        "authentication\n");
    free(fb_access_token);
    return 1;
  }

  strcpy(pa->pid, pid);
  strcpy(pa->access_token, access_token);
  strcpy(pa->fb_access_token, fb_access_token);

  worker_idle_add(auth_after, pa);

  free(fb_access_token);

  return 0;
}

void controller_auth_after(struct oauth2_context *ctx) {
  if (ctx->error_code != 0) {
    ERROR_NOTE("Failed to get the facebook access_token\n");
    controller_lock(0);
  // We have the access token we can move on the next steps of authentication
  } else {
    worker_run("auth_worker", auth_worker, strdup(ctx->access_token));
  }
}

void controller_auth(void) {
  int ret;
  DEBUG("Authenticate\n");
  // worker_run("auth_worker", auth_worker, NULL);
  controller_lock(1);
  ret = oauth2_get_access_token_async(FB_OAUTH2_URL, FB_OAUTH2_URL_CONFIRM,
      controller_auth_after);
  if (ret != 0) {
    ERROR_NOTE("Failed to start oauth2 process %d\n", ret);
  }
}
