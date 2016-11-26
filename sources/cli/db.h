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

#ifndef __DB_H__
#define __DB_H__

#include <pickup/pickup.h>

#define DB_FILENAME "pickup.db"

int db_init(void);
int db_cleanup(void);

int db_delete_match(const char *mid);
int db_delete_person(const char *pid);
int db_update_match(const struct pickup_match *m);
int db_update_rec(const struct pickup_match *m);
int db_update_message(const struct pickup_message *m, const char *mid);
int db_select_matches(void (*cb_match)(struct pickup_match *));
int db_select_recs(void (*cb_recs)(struct pickup_match *));
int db_select_match(const char *pid, struct pickup_match **match);
int db_select_rec(const char *pid, struct pickup_match **match);

#endif//__DB_H__
