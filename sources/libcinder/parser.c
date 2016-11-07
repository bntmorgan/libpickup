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
#include "parser.h"

const char *path_token[] = { "token", (const char *) 0 };
const char *path_user_id[] = { "user", "_id", (const char *) 0 };
const char *path_last_activity_date[] = { "last_activity_date",
    (const char *) 0 };
const char *path_matches[] = { "matches", (const char *) 0 };
const char *path_blocks[] = { "blocks", (const char *) 0 };
const char *path_mid[] = { "_id", (const char *) 0 };
const char *path_person[] = { "person", (const char *) 0 };
const char *path_pid[] = { "_id", (const char *) 0 };
const char *path_name[] = { "name", (const char *) 0 };
const char *path_date[] = { "created_date", (const char *) 0 };
const char *path_birth[] = { "birth_date", (const char *) 0 };
const char *path_img[] = { "photos", (const char *) 0 };
const char *path_img_id[] = { "id", (const char *) 0 };
const char *path_img_url[] = { "url", (const char *) 0 };
const char *path_img_processed[] = { "processedFiles", (const char *) 0 };
const char *path_img_height[] = { "height", (const char *) 0 };
const char *path_img_width[] = { "width", (const char *) 0 };
const char *path_messages[] = { "messages", (const char *) 0 };
const char *path_messages_id[] = { "_id", (const char *) 0 };
const char *path_messages_message[] = { "message", (const char *) 0 };
const char *path_messages_to[] = { "to", (const char *) 0 };
const char *path_messages_date[] = { "created_date", (const char *) 0 };
const char *path_results[] = { "results", (const char *) 0 };
const char *path_match_id[] = { "match", "_id", (const char *) 0 };
const char *path_is_new_message[] = { "is_new_message", (const char *) 0 };
const char *path_swipe_remaining[] = { "likes_remaining", (const char *) 0 };

int parser_auth(const char *buf, char *token, char *pid) {
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
    return CINDER_ERR;
  }

  yajl_val v = yajl_tree_get(node, path_token, yajl_t_string);
  if (v) {
    char *t = YAJL_GET_STRING(v);
    // Copy the token
    strcpy(token, t);
  } else {
    ERROR("no such node: %s\n", path_token[0]);
    return CINDER_ERR;
  }

  v = yajl_tree_get(node, path_user_id, yajl_t_string);
  if (v) {
    char *t = YAJL_GET_STRING(v);
    // Copy the id
    strcpy(pid, t);
  } else {
    ERROR("no such node: %s/%s\n", path_user_id[0], path_user_id[1]);
    return CINDER_ERR;
  }

  yajl_tree_free(node);

  return 0;
}

int parser_match_free(struct cinder_match *m) {
  if (m == NULL) {
    return CINDER_ERR;
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

int parser_message(yajl_val node, struct cinder_message *m) {
  yajl_val obj;
  char *t;
  const char *user_pid = cinder_get_pid();
  struct tm time;

  if (user_pid == NULL) {
    ERROR("No user pid, aborting\n");
    return CINDER_ERR;
  }

  // id
  obj = yajl_tree_get(node, path_messages_id, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_messages_id[0]);
    return CINDER_ERR;
  }
  t = YAJL_GET_STRING(obj);
  strcpy(&m->id[0], t);

  // message
  obj = yajl_tree_get(node, path_messages_message, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_messages_message[0]);
    return CINDER_ERR;
  }
  t = YAJL_GET_STRING(obj);
  strcpy(&m->message[0], t);

  // to
  obj = yajl_tree_get(node, path_messages_to, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_messages_to[0]);
    return CINDER_ERR;
  }
  t = YAJL_GET_STRING(obj);
  if (strncmp(user_pid, t, strlen(t)) == 0) {
    m->dir = CINDER_MESSAGE_INPUT;
  } else {
    m->dir = CINDER_MESSAGE_OUTPUT;
  }

  // date
  obj = yajl_tree_get(node, path_messages_date, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_messages_date[0]);
    return CINDER_ERR;
  } else {
    t = YAJL_GET_STRING(obj);
  }
  strptime(t, "%Y-%m-%dT%H:%M:%S.%z", &time);
  m->date = mktime(&time);  // timestamp in GMT

  return 0;
}

int parser_image(yajl_val node, struct cinder_image *img) {
  yajl_val obj, objv;
  char *t;

  // id
  obj = yajl_tree_get(node, path_img_id, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_img_id[0]);
    return CINDER_ERR;
  }
  t = YAJL_GET_STRING(obj);
  strcpy(&img->id[0], t);

  // url
  obj = yajl_tree_get(node, path_img_url, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_img_url[0]);
    return CINDER_ERR;
  }
  t = YAJL_GET_STRING(obj);
  strcpy(&img->url[0], t);

  // processed images array
  obj = yajl_tree_get(node, path_img_processed, yajl_t_array);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_img_processed[0]);
    return CINDER_ERR;
  }

  DEBUG("array found\n");
  size_t len = obj->u.array.len;
  int i;

  if (len != CINDER_SIZE_PROCESSED) {
    ERROR("There is more or less than %d processed images : %ld\n",
        CINDER_SIZE_PROCESSED, len);
    return CINDER_ERR;
  }

  // Iterate over all processed images
  for (i = 0; i < len; i++) {
    // url
    objv = yajl_tree_get(obj->u.array.values[i], path_img_url, yajl_t_string);
    if (objv == NULL) {
      ERROR("no such node: %s\n", path_img_url[0]);
      return CINDER_ERR;
    }
    t = YAJL_GET_STRING(objv);
    strcpy(&img->processed[i].url[0], t);
    // width
    objv = yajl_tree_get(obj->u.array.values[i], path_img_width, yajl_t_number);
    if (objv == NULL) {
      ERROR("no such node: %s\n", path_img_width[0]);
      return CINDER_ERR;
    }
    img->processed[i].width = YAJL_GET_INTEGER(objv);
    // height
    objv = yajl_tree_get(obj->u.array.values[i], path_img_height,
        yajl_t_number);
    if (objv == NULL) {
      ERROR("no such node: %s\n", path_img_height[0]);
      return CINDER_ERR;
    }
    img->processed[i].height = YAJL_GET_INTEGER(objv);
    DEBUG("Processed img : %d %d %s\n", img->processed[i].width,
        img->processed[i].height, img->processed[i].url);
  }

  return 0;
}

int parser_person(yajl_val node, struct cinder_match *m) {
  char *t;
  int i;
  yajl_val obj, objp;
  struct tm time;
  size_t slen;

  // pid
  obj = yajl_tree_get(node, path_pid, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s/%s\n", path_pid[0], path_pid[1]);
    return CINDER_ERR;
  }
  t = YAJL_GET_STRING(obj);
  strcpy(&m->pid[0], t);

  // name
  obj = yajl_tree_get(node, path_name, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s/%s\n", path_name[0], path_name[1]);
    return CINDER_ERR;
  } else {
    t = YAJL_GET_STRING(obj);
  }
  strcpy(&m->name[0], t);

  // birth
  obj = yajl_tree_get(node, path_birth, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s/%s\n", path_birth[0], path_birth[1]);
    return CINDER_ERR;
  } else {
    t = YAJL_GET_STRING(obj);
  }
  strptime(t, "%Y-%m-%dT%H:%M:%S.%z", &time);
  m->birth = mktime(&time);  // timestamp in GMT

  // Pictures
  obj = yajl_tree_get(node, path_img, yajl_t_array);
  if (obj == NULL) {
    ERROR("no photos for this match: %s/%s\n", path_img[0],
        path_img[1]);
    return CINDER_ERR;
  }

  DEBUG("array found\n");
  slen = obj->u.array.len;

  m->images_count = slen;
  m->images = malloc(sizeof(struct cinder_image) * slen);
  memset(m->images, 0, sizeof(struct cinder_image) * slen);

  if (m->images == NULL) {
    return CINDER_ERR_NO_MEM;
  }

  // Iterate over all matches to create images objects
  for (i = 0; i < slen; ++i) {
    DEBUG("img %d\n", i);
    objp = obj->u.array.values[i]; // image object
    if (parser_image(objp, &m->images[i]) != 0) {
      ERROR("failed to parse image\n");
      return CINDER_ERR;
    }
  }

  return 0;
}

int parser_match(yajl_val node, struct cinder_updates_callbacks *cb,
    void *data) {
  yajl_val obj, objp;
  char *t;
  size_t slen;
  int i;
  struct cinder_match *m;
  struct tm time;
  int is_new_message = 0;
  int ret;

  // Allocation
  m = malloc(sizeof(struct cinder_match));
  if (m == NULL) {
    ERROR("Failed to allocate a match\n");
    return CINDER_ERR;
  }
  memset(m, 0, sizeof(struct cinder_match));

  // mid
  obj = yajl_tree_get(node, path_mid, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_mid[0]);
    parser_match_free(m);
    return CINDER_ERR;
  }
  t = YAJL_GET_STRING(obj);
  strcpy(&m->mid[0], t);

  // New message ?
  obj = yajl_tree_get(node, path_is_new_message, yajl_t_any);
  if (obj == NULL) {
    DEBUG("no such node: %s\n", path_is_new_message[0]);
  } else {
    if (YAJL_IS_TRUE(obj)) {
      is_new_message = 1;
    }
  }

  // Messages
  obj = yajl_tree_get(node, path_messages, yajl_t_array);
  if (obj == NULL) {
    ERROR("no messages for this match: %s/\n", path_messages[0]);
    parser_match_free(m);
    return CINDER_ERR;
  }

  DEBUG("array found\n");
  slen = obj->u.array.len;

  m->messages_count = slen;
  m->messages = malloc(sizeof(struct cinder_message) * slen);
  memset(m->messages, 0, sizeof(struct cinder_message) * slen);

  if (m->messages == NULL) {
    parser_match_free(m);
    return CINDER_ERR_NO_MEM;
  }

  // Iterate over all matches to create messages objects
  for (i = 0; i < slen; ++i) {
    DEBUG("msg %d\n", i);
    objp = obj->u.array.values[i]; // image object
    ret = parser_message(objp, &m->messages[i]);
    if (ret != 0) {
      ERROR("failed to parse message\n");
      parser_match_free(m);
      return ret;
    }
  }

  // Person
  obj = yajl_tree_get(node, path_person, yajl_t_object);
  if (obj == NULL) {
    if (is_new_message == 1) {
      NOTE("We just have a new message here\n");
      // Finally call the callback
      if (cb->message != NULL) {
        ret = cb->message(m, data);
        DEBUG("RET %d\n", ret);
        if (ret != 0) {
          parser_match_free(m);
          return CINDER_ERR_CB;
        }
      }
      parser_match_free(m);
      return 0;
    } else {
      ERROR("no such node: %s\n", path_person[0]);
      parser_match_free(m);
      return CINDER_ERR;
    }
  }
  ret = parser_person(obj, m);
  if (ret != 0) {
    ERROR("Failed to parse person\n");
    parser_match_free(m);
    return ret;
  }

  // Date
  obj = yajl_tree_get(node, path_date, yajl_t_string);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_date[0]);
    return CINDER_ERR;
  } else {
    t = YAJL_GET_STRING(obj);
  }
  strptime(t, "%Y-%m-%dT%H:%M:%S.%z", &time);
  m->date = mktime(&time);  // timestamp in GMT

  // Finally call the callback
  if (cb->match != NULL) {
    if (cb->match(m, data) != 0) {
      return CINDER_ERR_CB;
    }
  }
  parser_match_free(m);
  return 0;
}

int parser_prepare_match(const char *buf, struct cinder_updates_callbacks *cb,
    void *data) {
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
    return CINDER_ERR;
  }

  // Likes remaining
  obj = yajl_tree_get(node, path_results, yajl_t_object);
  if (obj == NULL) {
    ERROR("no such node: %s\n", path_results[0]);
    return CINDER_ERR;
  }

  return parser_match(obj, cb, data);
}

int parser_prepare_message(const char *buf, struct cinder_message *msg) {
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
    return CINDER_ERR;
  }

  return parser_message(node, msg);
}

int parser_updates(const char *buf, struct cinder_updates_callbacks *cb,
    void *data, char *last_activity_date) {
  yajl_val node, obj, v;
  char errbuf[1024];
  char *t;
  size_t len;
  int i;

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
    return CINDER_ERR;
  }

  // Last activity date
  v = yajl_tree_get(node, path_last_activity_date, yajl_t_string);
  if (v == NULL) {
    ERROR("no such node: %s\n", path_date[0]);
    return CINDER_ERR;
  } else {
    t = YAJL_GET_STRING(v);
  }
  strcpy(last_activity_date, t);

  // Matches
  v = yajl_tree_get(node, path_matches, yajl_t_array);
  if (v == NULL) {
    ERROR("no such node: %s\n", path_matches[0]);
    return CINDER_ERR;
  }

  DEBUG("array found\n");

  len = v->u.array.len;

  // Iterate over all matches to create matches objects
  for (i = 0; i < len; ++i) {
    DEBUG("elt %d\n", i);

    obj = v->u.array.values[i]; // object

    int ret = parser_match(obj, cb, data);
    if (ret == CINDER_ERR_PARSE_FAILED) {
      ERROR("Failed to parse a match... but we continue...\n");
      continue;
    }
    if (ret == CINDER_ERR_CB) {
      ERROR("Failed to execute the callback fo a match ! Stopping here\n");
      return CINDER_ERR;
    }
  }

  // Blocks
  v = yajl_tree_get(node, path_blocks, yajl_t_array);
  if (v == NULL) {
    DEBUG("no such node: %s\n", path_blocks[0]);
    DEBUG("No new blocks\n");
    yajl_tree_free(node);
    return 0;
  }

  DEBUG("array found\n");

  len = v->u.array.len;

  // Iterate over all blocks
  for (i = 0; i < len; ++i) {
    DEBUG("elt %d\n", i);
    t = YAJL_GET_STRING(v->u.array.values[i]);
    if (t == NULL) {
      ERROR("Failed to parse a block\n");
      return CINDER_ERR;
    }
    if (cb->block != NULL) {
      if (cb->block(t, data) != 0) {
        yajl_tree_free(node);
        return CINDER_ERR_CB;
      }
    }
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
  int ret;

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
    return CINDER_ERR;
  }

  yajl_val v = yajl_tree_get(node, path_results, yajl_t_array);
  if (v == NULL) {
    ERROR("no such node: %s\n", path_results[0]);
    return CINDER_ERR;
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
      ret = parser_image(objp, &m->images[j]);
      if (ret != 0) {
        ERROR("failed to parse image\n");
        continue;
      }
    }

    // Finally call the callback
    if (cb->rec != NULL) {
      if (cb->rec(m, data) != 0) {
        yajl_tree_free(node);
        parser_match_free(m);
        return CINDER_ERR_CB;
      }
    }

    parser_match_free(m);
  }

  yajl_tree_free(node);

  return 0;
}

int parser_swipe(const char *buf, int *remaining_likes, char *id_match) {
  yajl_val node, obj;
  char errbuf[1024];
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
    return CINDER_ERR;
  }

  // Likes remaining
  obj = yajl_tree_get(node, path_swipe_remaining,
      yajl_t_number);
  if (obj == NULL) {
    DEBUG("no such node: %s\n", path_swipe_remaining[0]);
    *remaining_likes = CINDER_ERR;
  } else {
    *remaining_likes = YAJL_GET_INTEGER(obj);
  }

  // Is there a new match ?
  obj = yajl_tree_get(node, path_match_id, yajl_t_string);
  if (obj == NULL) {
    DEBUG("Not a match for the moment\n");
    return 0;
  } else {
    t = YAJL_GET_STRING(obj);
    DEBUG("New match %s !\n", t);
  }
  strcpy(id_match, t);

  return 0;
}
