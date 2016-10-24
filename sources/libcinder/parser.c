#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

const char * path_matches[] = { "matches", (const char *) 0 };
const char * path_id[] = { "_id", (const char *) 0 };
const char * path_name[] = { "person", "name", (const char *) 0 };
const char * path_age[] = { "person", "age", (const char *) 0 };

int parser_updates(const char *buf, struct cinder_updates *u) {
  yajl_val node, obj, objv;
  char errbuf[1024];

  memset(u, 0, sizeof(struct cinder_updates));

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

  printf("array found\n");

  size_t len = v->u.array.len;
  int i;

  // Iterate over all matches to create matches objects
  u->matches_count = len;
  u->matches = malloc(sizeof(struct cinder_match) * len);
  for (i = 0; i < len; ++i) {
    printf("elt %d\n", i);

    obj = v->u.array.values[i]; // object
    char *t;

    // id
    objv = yajl_tree_get(obj, path_id, yajl_t_string);
    if (objv == NULL) {
      fprintf(stderr, "no such node: %s\n", path_matches[0]);
      return -1;
    }
    t = YAJL_GET_STRING(objv);
    if (t == NULL) {
      return -1;
    }
    u->matches[i].id = strdup(t);

    // name
    objv = yajl_tree_get(obj, path_name, yajl_t_string);
    if (objv == NULL) {
      fprintf(stderr, "no such node: %s/%s\n", path_name[0], path_name[1]);
      t = "no name";
    } else {
      t = YAJL_GET_STRING(objv);
    }
    if (t == NULL) {
      return -1;
    }
    u->matches[i].name = strdup(t);

    // age
    u->matches[i].age = 0;

  }
  printf("Nb elt %i\n", i);

	yajl_tree_free(node);

  return 0;
}
