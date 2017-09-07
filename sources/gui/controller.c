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
#include "worker.h"

#define FB_TOKEN_NAME "pickup_fb_token"
#define PID_NAME "pickup_user_pid"
#define TOKEN_NAME "pickup_token"
#define LAST_ACTIVITY_DATE "last_activity_date"

/**
 * Static flags and vars
 */
static int auth = 0;
static char access_token[0x100];
static char pid[PICKUP_SIZE_ID];

void controller_cleanup(void) {
  model_cleanup();
  pickup_cleanup();
}

void controller_init(void) {
  pickup_init();
  model_init();
  model_populate();
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
}

int image_download(struct pickup_image *img, int i, char *pid) {
  size_t count;
  char *data;
  // XXX ext4 max file path length
  char filename[0x1000];
  // Create the filename
  // XXX .JPG
  sprintf(&filename[0], "%s_%s_%d.jpg", pid, img->id, i);
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

int image_gallery(struct pickup_image *img, int i, char *pid, char *path) {
  char filename[PICKUP_SIZE_ID * 2 + 10];
  // Create the filename
  // XXX .JPG
  snprintf(&filename[0], PICKUP_SIZE_ID * 2 + 10, "%s_%s_%d.jpg", pid,
      img->id, i);
  DEBUG("Filename %s\n", filename);
  // We check if we have to download it
  if (file_exists(filename, IO_PATH_CACHE_IMG) != 0) {
    DEBUG("Image %s not downloaded yet : we do it\n", filename);
    if (image_download(img, i, pid) != 0) {
      ERROR("Failed to download image %s\n", filename);
      return -1;
    }
  }
  // XXX ext4 max file path length
  if (path_resolve(filename, IO_PATH_CACHE_IMG, &path[0], 0x1000) != 0) {
    ERROR("Failed to resolve path for %s\n", filename);
    return -1;
  }
  return 0;
}

void controller_clear_match(void) {
  g_object_set(selected, "pid", "", "name", "", "birth", 0, "images", NULL,
      "images-count", 0, "image-index", 0, "image", "", "match", 0,
      "image-progress", 0, "index", 0, "set", 0, "lock", 0, NULL);
}

void set_match(struct pickup_match *m, int match, unsigned int index) {
  // XXX ext4 max file path length
  char path[0x1000];
  struct pickup_image *images;
  if (image_gallery(&m->images[0], 0, m->pid, &path[0])) {
    ERROR("Error while getting image to display\n");
    path[0] = '\0';
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
      "image-index", 0, "image", &path[0], "match", match, "image-progress",
      progress, "index", index, "set", 1, NULL);
}

void controller_image_skip(int skip) {
  // XXX ext4 max file path length
  char path[0x1000];
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
  if (image_gallery(&images[index], index, pid, &path[0])) {
    ERROR("Error while getting image to display\n");
    path[0] = '\0';
  }
  // Finally set the path
  progress = (gfloat) (index + 1)/ count;
  DEBUG("Progress %f\n", progress);
  g_object_set(selected, "image-index", index, "image", &path[0], NULL);
  g_object_set(selected, "image-progress", progress, NULL);
}

void controller_set_match(const char *pid, unsigned int index) {
  struct pickup_match *m;
  if (db_select_match(pid, &m)) {
    ERROR("Failed to match %s from db\n", pid);
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

struct cb_match_param {
  GPtrArray *matches;
};

int cb_match(struct pickup_match *m, void *data) {
  struct cb_match_param *p = data;
  printf("Update for match [%s]%s\n", m->pid, m->name);
  if (db_update_match(m) != 0) {
    ERROR("Failed to update the match\n");
    pickup_match_free(m);
    return -1;
  }
  MatchList *obj;
  obj = g_object_new(match_list_get_type(), "mid", m->mid, "pid", m->pid,
      "name", m->name, "date", m->date, "birth", m->birth, NULL);
  g_ptr_array_add(p->matches, obj);
  pickup_match_print(m);
  return 0;
}

struct swipe_rec_after_param {
  GPtrArray *matches;
  int ret;
  int like;
  int rl;
};

void swipe_rec_after(void *data) {
  struct swipe_rec_after_param *p = data;
  unsigned int index;
  if (p->matches->len == 0) {
    DEBUG("No new match\n");
    if ((p->rl > 0 && p->like == 1) || p->like == 0) {
      // Update model
      g_object_get(selected, "index", &index, NULL);
      DEBUG("Index to remove %d\n", index);
      g_list_store_remove(recs, index);
    }
  }
  g_ptr_array_free(p->matches, TRUE);
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
  struct cb_match_param pcb;
  pcb.matches = g_ptr_array_new();
  ret = pickup_swipe(p->pid, p->like, &rl, &cbu, &pcb);
  if (ret != 0) {
    ERROR("Failed to dislike %s\n", pid);
  }
  struct swipe_rec_after_param *pa = malloc(sizeof(struct
        swipe_rec_after_param));
  pa->ret = ret;
  pa->rl = rl;
  pa->matches = pcb.matches;
  pa->like = p->like;
  DEBUG("Is new match ? %d\n", pcb.matches->len);
  if (pcb.matches->len == 0) {
    DEBUG("Remaining likes %d, like ? %d\n", rl, p->like);
    if ((rl > 0 && p->like == 1) || p->like == 0) {
      DEBUG("We can remove person %s\n", pid);
      // We can remove the recommendation
      if (db_delete_person(pid) != 0) {
        ERROR("Failed to delete the recommendation\n");
      }
    }
  }
  free(p);
  return pa;
}

void controller_swipe_rec(int like) {
  struct swipe_rec_worker_param *p = malloc(sizeof(struct
        swipe_rec_worker_param));
  g_object_get(selected, "pid", &p->pid, NULL);
  p->like = like;
  DEBUG("Swiping rec[%s] like %d\n", pid, like);
  worker_run("swipe_rec_worker", swipe_rec_worker, p, swipe_rec_after);
}

int cb_rec(struct pickup_match *m, void *data) {
  DEBUG("New rec[%s]%s\n", m->pid, m->name);
  MatchList *obj;
  if (db_update_rec(m) != 0) {
    ERROR("Failed to update the rec\n");
    return -1;
  }
  pickup_match_print(m);
  obj = g_object_new(match_list_get_type(), "pid", m->pid,
      "name", m->name, "date", m->date, "birth", m->birth, NULL);
  g_list_store_append(recs, obj);
  return 0;
}

int controller_recs_scan(void) {
  struct pickup_recs_callbacks cbr = {
    cb_rec,
  };
  return pickup_recs(&cbr, NULL);
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

int controller_updates(void) {
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

int controller_message(char *text) {
  struct pickup_message msg;
  char *mid;
  g_object_get(selected, "mid", &mid, NULL);
  Message *message;
  DEBUG("Send message %s to %s\n", text, mid);
  if (pickup_message(&mid[0], text, &msg) != 0) {
    ERROR("Failed to send a message to %s\n", mid);
    return -1;
  }
  DEBUG("Add the sent message to the database \n");
  if (db_update_message(&msg, mid) != 0) {
    return -1;
  }
  // Add the message to the model
  message = g_object_new(message_get_type(), "id", msg.id,
      "dir", msg.dir, "date", msg.date,
      "message", msg.message, NULL);
  g_list_store_append(messages, message);
  return 0;
}

void controller_lock(int lock) {
  DEBUG("Lock %d\n", lock);
  g_object_set(selected, "lock", lock, NULL);
}
