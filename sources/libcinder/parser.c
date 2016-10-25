#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <cinder/cinder.h>

#include "yajl/yajl_tree.h"

int parser_token(const char *buf, char *token) {
  yajl_val node;
  char errbuf[1024];

  node = yajl_tree_parse(buf, errbuf, sizeof(errbuf));

  /* parse error handling */
  if (node == NULL) {
    fprintf(stderr, "parse_error: ");
    if (strlen(errbuf)) fprintf(stderr, " %s", errbuf);
    else fprintf(stderr, "unknown error");
    fprintf(stderr, "\n");
    return -1;
  }

  const char * path[] = { "token", (const char *) 0 };
  yajl_val v = yajl_tree_get(node, path, yajl_t_string);
  if (v) {
    char *t = YAJL_GET_STRING(v);
    // Copy the token
    strcpy(token, t);
  } else {
    fprintf(stderr, "no such node: %s/%s\n", path[0], path[1]);
    return -1;
  }

	yajl_tree_free(node);

  return 0;
}

const char *path_matches[] = { "matches", (const char *) 0 };
const char *path_id[] = { "person", "_id", (const char *) 0 };
const char *path_name[] = { "person", "name", (const char *) 0 };
const char *path_birth[] = { "person", "birth_date", (const char *) 0 };
const char *path_pic[] = { "person", "photos", (const char *) 0 };
const char *path_pic_url[] = { "url", (const char *) 0 };
const char *path_pic_processed[] = { "processedFiles", (const char *) 0 };
const char *path_pic_height[] = { "height", (const char *) 0 };
const char *path_pic_width[] = { "width", (const char *) 0 };
const char *path_messages[] = { "messages", (const char *) 0 };
const char *path_messages_message[] = { "message", (const char *) 0 };
const char *path_messages_to[] = { "to", (const char *) 0 };
const char *path_results[] = { "results", (const char *) 0 };

int parser_match_free(struct cinder_match *m) {
  if (m == NULL) {
    return -1;
  }

  // Pictures
  if (m->pictures != NULL) {
    free(m->pictures);
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

  // message
  obj = yajl_tree_get(node, path_messages_message, yajl_t_string);
  if (obj == NULL) {
    fprintf(stderr, "no such node: %s\n", path_messages_message[0]);
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
    fprintf(stderr, "no such node: %s\n", path_messages_to[0]);
    return -1;
  }
  t = YAJL_GET_STRING(obj);
  if (t == NULL) {
    return -1;
  }
  if (strncmp(match->id, t, strlen(t)) == 0) {
    m->dir = CINDER_MESSAGE_INPUT;
  } else {
    m->dir = CINDER_MESSAGE_OUTPUT;
  }
  return 0;
}

int parser_picture(yajl_val node, struct cinder_picture *p) {
  yajl_val obj, objv;
  char *t;

  // url
  obj = yajl_tree_get(node, path_pic_url, yajl_t_string);
  if (obj == NULL) {
    fprintf(stderr, "no such node: %s\n", path_pic_url[0]);
    return -1;
  }
  t = YAJL_GET_STRING(obj);
  if (t == NULL) {
    return -1;
  }
  strcpy(&p->url[0], t);

  // processed pictures array
  obj = yajl_tree_get(node, path_pic_processed, yajl_t_array);
  if (obj == NULL) {
    fprintf(stderr, "no such node: %s\n", path_pic_processed[0]);
    return -1;
  }

  // printf("array found\n");
  size_t len = obj->u.array.len;
  int i;

  if (len > 4) {
    fprintf(stderr, "to many processed files %ld\n", len);
    return -1;
  }

  // Iterate over all processed pictures
  for (i = 0; i < len; i++) {
    // url
    objv = yajl_tree_get(obj->u.array.values[i], path_pic_url, yajl_t_string);
    if (objv == NULL) {
      fprintf(stderr, "no such node: %s\n", path_pic_url[0]);
      return -1;
    }
    t = YAJL_GET_STRING(objv);
    if (t == NULL) {
      return -1;
    }
    strcpy(&p->processed[i].url[0], t);
    // width
    objv = yajl_tree_get(obj->u.array.values[i], path_pic_width, yajl_t_number);
    if (objv == NULL) {
      fprintf(stderr, "no such node: %s\n", path_pic_width[0]);
      return -1;
    }
    p->processed[i].width = YAJL_GET_INTEGER(objv);
    // height
    objv = yajl_tree_get(obj->u.array.values[i], path_pic_height,
        yajl_t_number);
    if (objv == NULL) {
      fprintf(stderr, "no such node: %s\n", path_pic_height[0]);
      return -1;
    }
    p->processed[i].height = YAJL_GET_INTEGER(objv);
  }

  return 0;
}

int parser_updates(const char *buf, struct cinder_updates_callbacks *cb, void
    *data) {
  yajl_val node, obj, objv, objp;
  char errbuf[1024];

  node = yajl_tree_parse(buf, errbuf, sizeof(errbuf));

  /* parse error handling */
  if (node == NULL) {
    fprintf(stderr, "parse_error: ");
    if (strlen(errbuf)) fprintf(stderr, " %s", errbuf);
    else fprintf(stderr, "unknown error");
    fprintf(stderr, "\n");
    return -1;
  }
  yajl_val v = yajl_tree_get(node, path_matches, yajl_t_array);
  if (v == NULL) {
    fprintf(stderr, "no such node: %s\n", path_matches[0]);
    return -1;
  }

  // printf("array found\n");

  size_t len = v->u.array.len;
  int i;

  // Iterate over all matches to create matches objects
  struct cinder_match *m;

  for (i = 0; i < len; ++i) {
    size_t slen, j;
    m = malloc(sizeof(struct cinder_match));
    memset(m, 0, sizeof(struct cinder_match));

    // printf("elt %d\n", i);

    obj = v->u.array.values[i]; // object
    char *t;

    // id
    objv = yajl_tree_get(obj, path_id, yajl_t_string);
    if (objv == NULL) {
      fprintf(stderr, "no such node: %s/%s\n", path_id[0], path_id[1]);
      parser_match_free(m);
      continue;
    }
    t = YAJL_GET_STRING(objv);
    if (t == NULL) {
      parser_match_free(m);
      continue;
    }
    strcpy(&m->id[0], t);

    // name
    objv = yajl_tree_get(obj, path_name, yajl_t_string);
    if (objv == NULL) {
      fprintf(stderr, "no such node: %s/%s\n", path_name[0], path_name[1]);
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

    // birth
    objv = yajl_tree_get(obj, path_birth, yajl_t_string);
    if (objv == NULL) {
      fprintf(stderr, "no such node: %s/%s\n", path_birth[0], path_birth[1]);
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
		// m->birth = timegm(&time);  // timestamp in current timezone
		m->birth = mktime(&time);  // timestamp in GMT

    // Pictures
    objv = yajl_tree_get(obj, path_pic, yajl_t_array);
    if (obj == NULL) {
      fprintf(stderr, "no photos for this match: %s/%s\n", path_pic[0],
          path_pic[1]);
      continue;
    }

    // printf("array found\n");
    slen = objv->u.array.len;

    m->pictures_count = slen;
    m->pictures = malloc(sizeof(struct cinder_picture) * slen);
    memset(m->pictures, 0, sizeof(struct cinder_picture) * slen);

    if (m->pictures == NULL) {
      parser_match_free(m);
      return CINDER_ERR_NO_MEM;
    }

    // Iterate over all matches to create pictures objects
    for (j = 0; j < slen; ++j) {
      // printf("pic %d\n", j);
      objp = objv->u.array.values[j]; // picture object
      if (parser_picture(objp, &m->pictures[j]) != 0) {
        fprintf(stderr, "failed to parse image\n");
        continue;
      }
    }

    // Messages
    objv = yajl_tree_get(obj, path_messages, yajl_t_array);
    if (obj == NULL) {
      fprintf(stderr, "no messages for this match: %s/\n", path_messages[0]);
      continue;
    }

    printf("array found\n");
    slen = objv->u.array.len;

    m->messages_count = slen;
    m->messages = malloc(sizeof(struct cinder_message) * slen);
    memset(m->messages, 0, sizeof(struct cinder_message) * slen);

    if (m->messages == NULL) {
      parser_match_free(m);
      return CINDER_ERR_NO_MEM;
    }

    // Iterate over all matches to create pictures objects
    for (j = 0; j < slen; ++j) {
      // printf("pic %d\n", j);
      objp = objv->u.array.values[j]; // picture object
      if (parser_message(objp, &m->messages[j], m) != 0) {
        fprintf(stderr, "failed to parse message\n");
        continue;
      }
    }

    // Finally call the callback
    cb->match(m, data);
  }

	yajl_tree_free(node);

  return 0;
}

const char *path_rec_id[] = { "_id", (const char *) 0 };
const char *path_rec_name[] = { "name", (const char *) 0 };
const char *path_rec_birth[] = { "birth_date", (const char *) 0 };
const char *path_rec_pic[] = { "photos", (const char *) 0 };

int parser_recs(const char *buf, struct cinder_recs_callbacks *cb, void *data) {
  yajl_val node, obj, objv, objp;
  char errbuf[1024];

  node = yajl_tree_parse(buf, errbuf, sizeof(errbuf));

  /* parse error handling */
  if (node == NULL) {
    fprintf(stderr, "parse_error: ");
    if (strlen(errbuf)) fprintf(stderr, " %s", errbuf);
    else fprintf(stderr, "unknown error");
    fprintf(stderr, "\n");
    return -1;
  }
  yajl_val v = yajl_tree_get(node, path_results, yajl_t_array);
  if (v == NULL) {
    fprintf(stderr, "no such node: %s\n", path_results[0]);
    return -1;
  }

  printf("array found\n");

  size_t len = v->u.array.len;
  int i;

  // Iterate over all recs to create recs objects
  struct cinder_match *m;

  for (i = 0; i < len; ++i) {
    size_t slen, j;
    m = malloc(sizeof(struct cinder_match));
    memset(m, 0, sizeof(struct cinder_match));

    // printf("elt %d\n", i);

    obj = v->u.array.values[i]; // object
    char *t;

    // id
    objv = yajl_tree_get(obj, path_rec_id, yajl_t_string);
    if (objv == NULL) {
      fprintf(stderr, "no such node: %s\n", path_rec_id[0]);
      parser_match_free(m);
      continue;
    }
    t = YAJL_GET_STRING(objv);
    if (t == NULL) {
      parser_match_free(m);
      continue;
    }
    strcpy(&m->id[0], t);

    // name
    objv = yajl_tree_get(obj, path_rec_name, yajl_t_string);
    if (objv == NULL) {
      fprintf(stderr, "no such node: %s\n", path_name[0]);
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

    // birth
    objv = yajl_tree_get(obj, path_rec_birth, yajl_t_string);
    if (objv == NULL) {
      fprintf(stderr, "no such node: %s\n", path_rec_birth[0]);
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
    objv = yajl_tree_get(obj, path_rec_pic, yajl_t_array);
    if (obj == NULL) {
      fprintf(stderr, "no photos for this rec: %s\n", path_pic[0]);
      continue;
    }

    printf("array found\n");
    slen = objv->u.array.len;

    m->pictures_count = slen;
    m->pictures = malloc(sizeof(struct cinder_picture) * slen);
    memset(m->pictures, 0, sizeof(struct cinder_picture) * slen);

    if (m->pictures == NULL) {
      parser_match_free(m);
      return CINDER_ERR_NO_MEM;
    }

    // Iterate over all recs to create pictures objects
    for (j = 0; j < slen; ++j) {
      // printf("pic %d\n", j);
      objp = objv->u.array.values[j]; // picture object
      if (parser_picture(objp, &m->pictures[j]) != 0) {
        fprintf(stderr, "failed to parse image\n");
        continue;
      }
    }

    // Finally call the callback
    cb->rec(m, data);
  }

	yajl_tree_free(node);

  return 0;
}
