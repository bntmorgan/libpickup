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

static struct {
  int auth;
  char access_token[0x100];
  char pid[PICKUP_SIZE_ID];
} user = {};

void controller_cleanup(void) {
  model_cleanup();
  pickup_cleanup();
}

void controller_init(void) {
  pickup_init();
  model_init();
  model_populate();
  // First ! We get the former access token in your pussy
  if (str_read(TOKEN_NAME, user.access_token, 0x100)) {
    NOTE("No access token found in dir ~/%s\n", IO_CONFIG_DIR);
  } else {
    NOTE("Access token found is %s\n", &user.access_token[0]);
    if (str_read(PID_NAME, user.pid, 0x100)) {
      NOTE("No access token found in dir ~/%s\n", IO_CONFIG_DIR);
    } else {
      NOTE("User pid found is %s\n", &user.pid[0]);
      // Set the access token and pid
      pickup_set_access_token(&user.access_token[0], user.pid);
      user.auth = 1;
    }
  }
}

#define IMG_FILENAME_MAX PICKUP_SIZE_ID * 2 + 10

void image_after(void *data) {
  char path[MAX_FILE_PATH];
  char *filename = data;
  DEBUG("Image %s downloaded : we set it\n", filename);
  // No worker
  if (path_resolve(filename, IO_PATH_CACHE_IMG, &path[0],
        MAX_FILE_PATH) != 0) {
    ERROR("Failed to resolve path for %s\n", filename);
    free(filename);
    return;
  }
  g_object_set(selected, "image", &path[0], NULL);
  free(filename);
}

struct image_download_param {
  struct pickup_image img;
  char filename[IMG_FILENAME_MAX];
};

void *image_download(void *data) {
  struct image_download_param *p = data;
  size_t count;
  char *buf;
  NOTE("Downloading image %s\n", p->filename);
  if (http_download_file(p->img.url, &buf, &count) != 0) {
    ERROR("Failed to download image %s\n", p->img.url);
    free(p);
    return NULL;
  }
  DEBUG("Size of %s : %d bytes\n", p->img.url, count);
  // Write the image to disk
  if (file_write(p->filename, IO_PATH_CACHE_IMG, buf, count) != 0) {
    ERROR("Failed to write image %s to disk\n", p->filename);
    free(buf);
    free(p);
    return NULL;
  }
  // We set image path in with an idle after
  worker_idle_add(image_after, strdup(p->filename));
  free(buf);
  free(p);
  return NULL;
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
    worker_run("image_download", image_download, p);
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
  g_object_set(selected, "pid", "", "name", "", "birth", 0, "images", NULL,
      "images-count", 0, "image-index", 0, "image", "", "match", 0,
      "image-progress", 0, "index", 0, "set", 0, "lock", 0, NULL);
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
  gint index, count;
  gchar *pid;
  gfloat progress;
  struct pickup_image *images;
  DEBUG("Skipping %d images\n", skip);
  g_object_get(selected, "image-index", &index, "images-count", &count,
      "images", &images, "pid", &pid, NULL);
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

void cb_match_idle(void *data) {
  struct pickup_match *m = data;
  MatchList *obj;
  obj = g_object_new(match_list_get_type(), "mid", m->mid, "pid", m->pid,
      "name", m->name, "date", m->date, "birth", m->birth, NULL);
  g_list_store_append(matches, obj);
  pickup_match_free(m);
}

int cb_match(struct pickup_match *m, void *data) {
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
  worker_idle_add(cb_match_idle, cm);
  return 0;
}

struct swipe_rec_after_param {
  int ret;
  int like;
  int rl;
  char *pid;
};

void swipe_rec_after(void *data) {
  struct swipe_rec_after_param *p = data;
  unsigned int index;
  DEBUG("Remaining likes %d, like ? %d\n", p->rl, p->like);
  if ((p->rl > 0 && p->like == 1) || p->like == 0) {
    DEBUG("We can remove person %s\n", p->pid);
    // We can remove the recommendation
    if (db_delete_person(p->pid) != 0) {
      ERROR("Failed to delete the recommendation\n");
    }
    // Update model
    g_object_get(selected, "index", &index, NULL);
    DEBUG("Index to remove %d\n", index);
    g_list_store_remove(recs, index);
  }
  free(p);
}

struct swipe_rec_worker_param {
  int like;
  char *pid;
};

void *swipe_rec_worker(void *data) {
  int rl = 0;
  int ret;
  struct swipe_rec_worker_param *p = data;
  struct pickup_updates_callbacks cbu = {
    cb_match,
  };
  ret = pickup_swipe(p->pid, p->like, &rl, &cbu, NULL);
  if (ret != 0) {
    ERROR_NOTE_WORKER("Failed to dislike %s\n", p->pid);
  }
  struct swipe_rec_after_param *pa = malloc(sizeof(struct
        swipe_rec_after_param));
  pa->ret = ret;
  pa->rl = rl;
  pa->like = p->like;
  pa->pid = p->pid;
  free(p);
  worker_idle_add(swipe_rec_after, pa);
  return NULL;
}

void controller_swipe_rec(int like) {
  struct swipe_rec_worker_param *p = malloc(sizeof(struct
        swipe_rec_worker_param));
  g_object_get(selected, "pid", &p->pid, NULL);
  p->like = like;
  DEBUG("Swiping rec[%s] like %d\n", p->pid, like);
  worker_run("swipe_rec_worker", swipe_rec_worker, p);
}

void cb_rec_idle(void *data) {
  MatchList *obj;
  struct pickup_match *m = data;
  obj = g_object_new(match_list_get_type(), "pid", m->pid,
      "name", m->name, "date", m->date, "birth", m->birth, NULL);
  g_list_store_append(recs, obj);
  pickup_match_free(m);
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

void *recs_scan_worker(void *data) {
  struct pickup_recs_callbacks cbr = {
    cb_rec,
  };
  if (pickup_recs(&cbr, NULL)) {
    ERROR_NOTE_WORKER("Failed to scan new recs\n");
  }
  return NULL;
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

void *updates_worker(void *data) {
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
  if (pickup_updates(&cbu, NULL, &last_activity_date[0]) != 0) {
    ERROR_NOTE_WORKER("Failed to get the updates\n");
    return NULL;
  }
  NOTE("Last activity %s\n", last_activity_date);
  if (str_write(LAST_ACTIVITY_DATE, &last_activity_date[0]) != 0) {
    ERROR_NOTE_WORKER("Failed to write last activity date\n");
    return NULL;
  }
  return NULL;
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

void message_after(void *data) {
  struct message_after_param *p = data;
  Message *message;
  // Add the message to the model
  message = g_object_new(message_get_type(), "id", p->msg.id,
      "dir", p->msg.dir, "date", p->msg.date,
      "message", p->msg.message, NULL);
  g_list_store_append(messages, message);
  free(p);
}

struct message_worker_param {
  char text[PICKUP_SIZE_MESSAGE];
  char *mid;
};

void *message_worker(void *data) {
  struct message_worker_param *p = data;
  struct message_after_param *pa;
  struct pickup_message msg;
  DEBUG("Send message %s to %s\n", p->text, p->mid);
  if (pickup_message(p->mid, p->text, &msg) != 0) {
    ERROR_NOTE_WORKER("Failed to send a message to %s\n", p->mid);
    free(p);
    return NULL;
  }
  pa = malloc(sizeof(struct pickup_message));
  if (pa == NULL) {
    free(p);
    return NULL;
  }
  memcpy(&pa->msg, &msg, sizeof(struct pickup_message));
  DEBUG("Add the sent message to the database \n");
  if (db_update_message(&msg, p->mid) != 0) {
    return NULL;
  }
  free(p);
  worker_idle_add(message_after, pa);
  return NULL;
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

void note_add_idle(void *data) {
  struct note_add_idle_param *p = data;
  note_add(p->type, p->msg);
  free(p->msg);
  free(p);
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

void *match_update_worker(void *data) {
  char *mid = data;
  struct pickup_updates_callbacks cb = {
    cb_match,
    cb_message,
  };
  if (pickup_get_match(mid, &cb, NULL)) {
    ERROR_NOTE_WORKER("Failed to update match %s\n", mid);
    return NULL;
  }
  return NULL;
}

void controller_match_update(void) {
  char *mid;
  g_object_get(selected, "mid", &mid, NULL);
  DEBUG("Update match mid(%s)\n", mid);
  worker_run("match_update_worker", match_update_worker, mid);
}
