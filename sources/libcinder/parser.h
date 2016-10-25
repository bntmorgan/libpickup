#ifndef __PARSER_H__
#define __PARSER_H__

#include <cinder/cinder.h>

int parser_token(const char *buf, char *token);
int parser_updates(const char *buf, struct cinder_updates_callbacks *u, void
    *data);
int parser_match_free(struct cinder_match *m);
int parser_swipe(const char *buf, unsigned int *remaining_likes);
int parser_recs(const char *buf, struct cinder_recs_callbacks *cb, void *data);

#endif//__PARSER_H__
