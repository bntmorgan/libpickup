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

#ifndef __PARSER_H__
#define __PARSER_H__

#include <cinder/cinder.h>
#include <time.h>

int parser_auth(const char *buf, char *token, char *pid);
int parser_updates(const char *buf, struct cinder_updates_callbacks *u,
    void *data, char *last_activity_date);
int parser_match_free(struct cinder_match *m);
int parser_swipe(const char *buf, unsigned int *remaining_likes,
    char *id_match);
int parser_recs(const char *buf, struct cinder_recs_callbacks *cb, void *data);
int parser_prepare_match(const char *buf, struct cinder_updates_callbacks *cb,
    void *data);

#endif//__PARSER_H__
