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

void controller_init(void) {
  model_init();
  model_populate();
}

void controller_destroy(void) {
  model_destroy();
}

int image_download(struct pickup_image *img, int i, struct pickup_match *m) {
  size_t count;
  char *data;
  // XXX ext4 max file path length
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
    char *path) {
  char filename[PICKUP_SIZE_ID * 2 + 10];
  // Create the filename
  // XXX .JPG
  snprintf(&filename[0], PICKUP_SIZE_ID * 2 + 10, "%s_%s_%d.jpg", m->pid,
      m->images[i].id, i);
  DEBUG("Filename %s\n", filename);
  // We check if we have to download it
  if (file_exists(filename, IO_PATH_CACHE_IMG) != 0) {
    DEBUG("Image %s not downloaded yet : we do it\n", filename);
    if (image_download(img, i, m) != 0) {
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
  image_gallery(&m->images[0], 0, m, &path[0]);
  g_object_set(selected, "pid", m->pid, "name", m->name, "birth", m->birth,
      "images", &m->images[0], "images_count", m->images_count, "image_index",
      0, "image", &path[0], NULL);
}

void controller_image_skip(int skip) {
  gint index, count;
  DEBUG("Skipping %d images\n", skip);
  g_object_get(selected, "image_index", &index, "images_count", &count, NULL);
  DEBUG("Current image index %d / %d\n", index, count);
}

void controller_set_match(const char *pid) {
  struct pickup_match *m;
  db_select_match(pid, &m);
  set_match(m);
  pickup_match_free(m);
}

void controller_set_rec(const char *pid) {
  struct pickup_match *m;
  db_select_rec(pid, &m);
  set_match(m);
  pickup_match_free(m);
}
