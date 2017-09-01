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

void controller_init(void) {
  model_init();
  model_populate();
}

void controller_destroy(void) {
  model_destroy();
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

void set_match(struct pickup_match *m) {
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
  // Set match attributes
  g_object_set(selected, "pid", m->pid, "name", m->name, "birth", m->birth,
      "images", &images[0], "images_count", m->images_count, "image_index",
      0, "image", &path[0], NULL);
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
}

void controller_image_skip(int skip) {
  // XXX ext4 max file path length
  char path[0x1000];
  gint index, count;
  gchar *pid;
  struct pickup_image *images;
  DEBUG("Skipping %d images\n", skip);
  g_object_get(selected, "image_index", &index, "images_count", &count,
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
  g_object_set(selected, "image_index", index, "image", &path[0], NULL);
}

void controller_set_match(const char *pid) {
  struct pickup_match *m;
  if (db_select_match(pid, &m)) {
    ERROR("Failed to match %s from db\n", pid);
    return;
  }
  set_match(m);
  pickup_match_free(m);
}

void controller_set_rec(const char *pid) {
  struct pickup_match *m;
  if (db_select_rec(pid, &m)) {
    ERROR("Failed to rec %s from db\n", pid);
    return;
  }
  set_match(m);
  pickup_match_free(m);
}
