#ifndef __PARSER_H__
#define __PARSER_H__

#include <cinder/cinder.h>

int parser_token(const char *buf, char *token);
int parser_updates(const char *buf, struct cinder_updates_callbacks *u, void
    *data);
int parser_match_free(struct cinder_match *m);

#endif//__PARSER_H__
