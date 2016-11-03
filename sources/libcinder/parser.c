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
#include <stdlib.h>
#include <time.h>

#include <cinder/cinder.h>
#include <yajl/yajl_tree.h>

#include "log.h"

int parser_token(const char *buf, char *token) {
  yajl_val node;
  char errbuf[1024];

  node = yajl_tree_parse(buf, errbuf, sizeof(errbuf));

  /* parse error handling */
  if (node == NULL) {
    ERROR("parse_error: ");
    if (strlen(errbuf)) {
      ERROR_RAW(" %s", errbuf);
    }
    else {
      ERROR_RAW("unknown error");
    }
    ERROR_RAW("\n");
    return -1;
  }

  const char * path[] = { "token", (const char *) 0 };
  yajl_val v = yajl_tree_get(node, path, yajl_t_string);
  if (v) {
    char *t = YAJL_GET_STRING(v);
    // Copy the token
    strcpy(token, t);
  } else {
    ERROR("no such node: %s/%s\n", path[0], path[1]);
    return -1;
  }

  yajl_tree_free(node);

  return 0;
}

const char *path_last_activity_date[] = { "last_activity_date",
    (const char *) 0 };
const char *path_matches[] = { "matches", (const char *) 0 };
const char *path_mid[] = { "_id", (const char *) 0 };
const char *path_pid[] = { "person", "_id", (const char *) 0 };
const char *path_name[] = { "person", "name", (const char *) 0 };
const char *path_date[] = { "created_date", (const char *) 0 };
const char *path_birth[] = { "person", "birth_date", (const char *) 0 };
const char *path_img[] = { "person", "photos", (const char *) 0 };
const char *path_img_id[] = { "id", (const char *) 0 };
const char *path_img_url[] = { "url", (const char *) 0 };
const char *path_img_processed[] = { "processedFiles", (const char *) 0 };
const char *path_img_height[] = { "height", (const char *) 0 };
const char *path_img_width[] = { "width", (const char *) 0 };
const char *path_messages[] = { "messages", (const char *) 0 };
const char *path_messages_id[] = { "_id", (const char *) 0 };
const char *path_messages_message[] = { "message", (const char *) 0 };
const char *path_messages_to[] = { "to", (const char *) 0 };
const char *path_messages_date[] = { "timestamp", (const char *) 0 };
const char *path_results[] = { "results", (const char *) 0 };

int parser_match_free(struct cinder_match *m) {
  if (m == NULL) {
    return -1;
  }

  // Pictures
  if (m->images != NULL) {
    free(m->images);
  }

  // Messages
  if (m->messages != NULL) {
    free(m->messages);
  }

  // Finally
  free(m);
  return 0;
}

int parser_message(yajl_val node, struct cinder_message *m, struct cinder_match
    *match) {
  yajl_val obj;
  char *t;

  // id
  obj = yajl_tree_get(node, path_messages_id, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_messages_id[0]);
    return -1;
  }
  t = YAJL_GET_STRING(obj);
  if (t == NULL) {
    return -1;
  }
  strcpy(&m->id[0], t);

  // message
  obj = yajl_tree_get(node, path_messages_message, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_messages_message[0]);
    return -1;
  }
  t = YAJL_GET_STRING(obj);
  if (t == NULL) {
    return -1;
  }
  strcpy(&m->message[0], t);

  // to
  obj = yajl_tree_get(node, path_messages_to, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_messages_to[0]);
    return -1;
  }
  t = YAJL_GET_STRING(obj);
  if (t == NULL) {
    return -1;
  }
  if (strcmp(match->pid, t) == 0) {
    m->dir = CINDER_MESSAGE_OUTPUT;
  } else {
    m->dir = CINDER_MESSAGE_INPUT;
  }

  // date
  obj = yajl_tree_get(node, path_messages_date, yajl_t_number);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_messages_date[0]);
    return -1;
  }
  m->date = YAJL_GET_INTEGER(obj);

  return 0;
}

int parser_image(yajl_val node, struct cinder_image *img) {
  yajl_val obj, objv;
  char *t;

  // id
  obj = yajl_tree_get(node, path_img_id, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_img_id[0]);
    return -1;
  }
  t = YAJL_GET_STRING(obj);
  if (t == NULL) {
    return -1;
  }
  strcpy(&img->id[0], t);

  // url
  obj = yajl_tree_get(node, path_img_url, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_img_url[0]);
    return -1;
  }
  t = YAJL_GET_STRING(obj);
  if (t == NULL) {
    return -1;
  }
  strcpy(&img->url[0], t);

  // processed images array
  obj = yajl_tree_get(node, path_img_processed, yajl_t_array);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_img_processed[0]);
    return -1;
  }

  DEBUG("array found\n");
  size_t len = obj->u.array.len;
  int i;

  if (len != CINDER_SIZE_PROCESSED) {
    ERROR("There is more or less than %d processed images : %ld\n",
        CINDER_SIZE_PROCESSED, len);
    return -1;
  }

  // Iterate over all processed images
  for (i = 0; i < len; i++) {
    // url
    objv = yajl_tree_get(obj->u.array.values[i], path_img_url, yajl_t_string);
    if (objv == NULL) {
      ERROR("no such node: %s\n", path_img_url[0]);
      return -1;
    }
    t = YAJL_GET_STRING(objv);
    if (t == NULL) {
      return -1;
    }
    strcpy(&img->processed[i].url[0], t);
    // width
    objv = yajl_tree_get(obj->u.array.values[i], path_img_width, yajl_t_number);
    if (objv == NULL) {
      ERROR("no such node: %s\n", path_img_width[0]);
      return -1;
    }
    img->processed[i].width = YAJL_GET_INTEGER(objv);
    // height
    objv = yajl_tree_get(obj->u.array.values[i], path_img_height,
        yajl_t_number);
    if (objv == NULL) {
      ERROR("no such node: %s\n", path_img_height[0]);
      return -1;
    }
    img->processed[i].height = YAJL_GET_INTEGER(objv);
    DEBUG("Processed img : %d %d %s\n", img->processed[i].width,
        img->processed[i].height, img->processed[i].url);
  }

  return 0;
}

int parser_updates(const char *buf, struct cinder_updates_callbacks *cb,
    void *data, time_t *last_activity_date) {
  yajl_val node, obj, objv, objp, v;
  char errbuf[1024];
  struct tm time;
  char *t;

  node = yajl_tree_parse(buf, errbuf, sizeof(errbuf));

  /* parse error handling */
  if (node == NULL) {
    ERROR("parse_error: ");
    if (strlen(errbuf)) {
      ERROR_RAW(" %s", errbuf);
    }
    else {
      ERROR_RAW("unknown error");
    }
    ERROR_RAW("\n");
    return -1;
  }

  // Last activity date
  v = yajl_tree_get(node, path_last_activity_date, yajl_t_string);
  if (v == NULL) {
    ERROR("no such node: %s\n", path_date[0]);
    return -1;
  } else {
    t = YAJL_GET_STRING(v);
  }
  if (t == NULL) {
    ERROR("Cannot convert to string\n");
    return -1;
  }
  strptime(t, "%Y-%m-%dT%H:%M:%S.%z", &time);
  *last_activity_date = mktime(&time);  // timestamp in GMT

  v = yajl_tree_get(node, path_matches, yajl_t_array);
  if (v == NULL) {
    ERROR("no such node: %s\n", path_matches[0]);
    return -1;
  }

  DEBUG("array found\n");

  size_t len = v->u.array.len;
  int i;

  // Iterate over all matches to create matches objects
  struct cinder_match *m;

  for (i = 0; i < len; ++i) {
    size_t slen, j;
    m = malloc(sizeof(struct cinder_match));
    memset(m, 0, sizeof(struct cinder_match));

    DEBUG("elt %d\n", i);

    obj = v->u.array.values[i]; // object

    // mid
    objv = yajl_tree_get(obj, path_mid, yajl_t_string);
    if (objv == NULL) {
      ERROR("no such node: %s\n", path_mid[0]);
      parser_match_free(m);
      continue;
    }
    t = YAJL_GET_STRING(objv);
    if (t == NULL) {
      parser_match_free(m);
      continue;
    }
    strcpy(&m->mid[0], t);

    // pid
    objv = yajl_tree_get(obj, path_pid, yajl_t_string);
    if (objv == NULL) {
      ERROR("no such node: %s/%s\n", path_pid[0], path_pid[1]);
      parser_match_free(m);
      continue;
    }
    t = YAJL_GET_STRING(objv);
    if (t == NULL) {
      parser_match_free(m);
      continue;
    }
    strcpy(&m->pid[0], t);

    // name
    objv = yajl_tree_get(obj, path_name, yajl_t_string);
    if (objv == NULL) {
      ERROR("no such node: %s/%s\n", path_name[0], path_name[1]);
      parser_match_free(m);
      continue;
    } else {
      t = YAJL_GET_STRING(objv);
    }
    if (t == NULL) {
      parser_match_free(m);
      continue;
    }
    strcpy(&m->name[0], t);

    // date
    objv = yajl_tree_get(obj, path_date, yajl_t_string);
    if (objv == NULL) {
      ERROR("no such node: %s\n", path_date[0]);
      parser_match_free(m);
      continue;
    } else {
      t = YAJL_GET_STRING(objv);
    }
    if (t == NULL) {
      parser_match_free(m);
      continue;
    }
    strptime(t, "%Y-%m-%dT%H:%M:%S.%z", &time);
    m->date = mktime(&time);  // timestamp in GMT

    // birth
    objv = yajl_tree_get(obj, path_birth, yajl_t_string);
    if (objv == NULL) {
      ERROR("no such node: %s/%s\n", path_birth[0], path_birth[1]);
      parser_match_free(m);
      continue;
    } else {
      t = YAJL_GET_STRING(objv);
    }
    if (t == NULL) {
      parser_match_free(m);
      continue;
    }
    strptime(t, "%Y-%m-%dT%H:%M:%S.%z", &time);
    m->birth = mktime(&time);  // timestamp in GMT

    // Pictures
    objv = yajl_tree_get(obj, path_img, yajl_t_array);
    if (obj == NULL) {
      ERROR("no photos for this match: %s/%s\n", path_img[0],
          path_img[1]);
      continue;
    }

    DEBUG("array found\n");
    slen = objv->u.array.len;

    m->images_count = slen;
    m->images = malloc(sizeof(struct cinder_image) * slen);
    memset(m->images, 0, sizeof(struct cinder_image) * slen);

    if (m->images == NULL) {
      parser_match_free(m);
      return CINDER_ERR_NO_MEM;
    }

    // Iterate over all matches to create images objects
    for (j = 0; j < slen; ++j) {
      DEBUG("img %d\n", j);
      objp = objv->u.array.values[j]; // image object
      if (parser_image(objp, &m->images[j]) != 0) {
        ERROR("failed to parse image\n");
        continue;
      }
    }

    // Messages
    objv = yajl_tree_get(obj, path_messages, yajl_t_array);
    if (obj == NULL) {
      ERROR("no messages for this match: %s/\n", path_messages[0]);
      continue;
    }

    DEBUG("array found\n");
    slen = objv->u.array.len;

    m->messages_count = slen;
    m->messages = malloc(sizeof(struct cinder_message) * slen);
    memset(m->messages, 0, sizeof(struct cinder_message) * slen);

    if (m->messages == NULL) {
      parser_match_free(m);
      return CINDER_ERR_NO_MEM;
    }

    // Iterate over all matches to create images objects
    for (j = 0; j < slen; ++j) {
      DEBUG("img %d\n", j);
      objp = objv->u.array.values[j]; // image object
      if (parser_message(objp, &m->messages[j], m) != 0) {
        ERROR("failed to parse message\n");
        continue;
      }
    }

    // Finally call the callback
    cb->match(m, data);
  }

  yajl_tree_free(node);

  return 0;
}

const char *path_rec_pid[] = { "_id", (const char *) 0 };
const char *path_rec_name[] = { "name", (const char *) 0 };
const char *path_rec_birth[] = { "birth_date", (const char *) 0 };
const char *path_rec_img[] = { "photos", (const char *) 0 };

int parser_recs(const char *buf, struct cinder_recs_callbacks *cb, void *data) {
  yajl_val node, obj, objv, objp;
  char errbuf[1024];

  node = yajl_tree_parse(buf, errbuf, sizeof(errbuf));

  /* parse error handling */
  if (node == NULL) {
    ERROR("parse_error: ");
    if (strlen(errbuf)) {
      ERROR_RAW(" %s", errbuf);
    }
    else {
      ERROR_RAW("unknown error");
    }
    ERROR_RAW("\n");
    return -1;
  }

  yajl_val v = yajl_tree_get(node, path_results, yajl_t_array);
  if (v == NULL) {
    ERROR("no such node: %s\n", path_results[0]);
    return -1;
  }

  DEBUG("array found\n");

  size_t len = v->u.array.len;
  int i;

  // Iterate over all recs to create recs objects
  struct cinder_match *m;

  for (i = 0; i < len; ++i) {
    size_t slen, j;
    m = malloc(sizeof(struct cinder_match));
    memset(m, 0, sizeof(struct cinder_match));

    DEBUG("elt %d\n", i);

    obj = v->u.array.values[i]; // object
    char *t;

    // pid
    objv = yajl_tree_get(obj, path_rec_pid, yajl_t_string);
    if (objv == NULL) {
      ERROR("no such node: %s\n", path_rec_pid[0]);
      parser_match_free(m);
      continue;
    }
    t = YAJL_GET_STRING(objv);
    if (t == NULL) {
      parser_match_free(m);
      continue;
    }
    strcpy(&m->pid[0], t);

    // name
    objv = yajl_tree_get(obj, path_rec_name, yajl_t_string);
    if (objv == NULL) {
      ERROR("no such node: %s\n", path_name[0]);
      parser_match_free(m);
      continue;
    } else {
      t = YAJL_GET_STRING(objv);
    }
    if (t == NULL) {
      parser_match_free(m);
      continue;
    }
    strcpy(&m->name[0], t);

    // Creation date : we add it ourselves
    time_t now = time(NULL);
    struct tm tm = *localtime(&now);
    m->date = mktime(&tm);

    // birth
    objv = yajl_tree_get(obj, path_rec_birth, yajl_t_string);
    if (objv == NULL) {
      ERROR("no such node: %s\n", path_rec_birth[0]);
      parser_match_free(m);
      continue;
    } else {
      t = YAJL_GET_STRING(objv);
    }
    if (t == NULL) {
      parser_match_free(m);
      continue;
    }
    struct tm time;
    strptime(t, "%Y-%m-%dT%H:%M:%S.%z", &time);
    m->birth = mktime(&time);  // timestamp in GMT

    // Pictures
    objv = yajl_tree_get(obj, path_rec_img, yajl_t_array);
    if (obj == NULL) {
      ERROR("no photos for this rec: %s\n", path_img[0]);
      continue;
    }

    DEBUG("array found\n");
    slen = objv->u.array.len;

    m->images_count = slen;
    m->images = malloc(sizeof(struct cinder_image) * slen);
    memset(m->images, 0, sizeof(struct cinder_image) * slen);

    if (m->images == NULL) {
      parser_match_free(m);
      return CINDER_ERR_NO_MEM;
    }

    // Iterate over all recs to create images objects
    for (j = 0; j < slen; ++j) {
      DEBUG("img %d\n", j);
      objp = objv->u.array.values[j]; // image object
      if (parser_image(objp, &m->images[j]) != 0) {
        ERROR("failed to parse image\n");
        continue;
      }
    }

    // Finally call the callback
    cb->rec(m, data);
  }

  yajl_tree_free(node);

  return 0;
}

const char *path_swipe_remaining[] = { "likes_remaining", (const char *) 0 };

int parser_swipe(const char *buf, unsigned int *remaining_likes) {
  yajl_val node, obj;
  char errbuf[1024];

  node = yajl_tree_parse(buf, errbuf, sizeof(errbuf));

  /* parse error handling */
  if (node == NULL) {
    ERROR("parse_error: ");
    if (strlen(errbuf)) {
      ERROR_RAW(" %s", errbuf);
    }
    else {
      ERROR_RAW("unknown error");
    }
    ERROR_RAW("\n");
    return -1;
  }

  obj = yajl_tree_get(node, path_swipe_remaining,
      yajl_t_number);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_swipe_remaining[0]);
    return -1;
  }
  *remaining_likes = YAJL_GET_INTEGER(obj);

  return 0;
}
