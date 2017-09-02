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

#include <glib.h>
#include <gio/gio.h>

#include <pickup/pickup.h>

#include "model.h"
#include "match_list.h"
#include "match.h"
#include "message.h"

#include "db.h"
#include "log.h"

GListStore *matches;
GListStore *recs;
GListStore *messages;
Match *selected;

static void cb_match(struct pickup_match *m) {
  DEBUG("Selected mid(%s)\n", m->mid);
  // Update model
  MatchList *obj;
  pickup_match_print(m);
  obj = g_object_new(match_list_get_type(), "mid", m->mid, "pid", m->pid,
      "name", m->name, "date", m->date, "birth", m->birth, NULL);
  g_list_store_append(matches, obj);
}

static void cb_recs(struct pickup_match *m) {
  DEBUG("Selected pid(%s)\n", m->pid);
  // Update model
  MatchList *obj;
  pickup_match_print(m);
  obj = g_object_new(match_list_get_type(), "pid", m->pid,
      "name", m->name, "date", m->date, "birth", m->birth, NULL);
  g_list_store_append(recs, obj);
}

void model_init(void) {
  db_init();
  // Lists models
  matches = g_list_store_new(match_list_get_type());
  recs = g_list_store_new(match_list_get_type());
  messages = g_list_store_new(message_get_type());
  // Full view model
  selected = g_object_new(match_get_type(), NULL);
}

void model_populate(void) {
  // Get all the recorded information
  if (db_select_matches(&cb_match)) {
    ERROR("Failed to access to db to get recorded matches\n");
  }
  if (db_select_recs(&cb_recs)) {
    ERROR("Failed to access to db to get recorded recs\n");
  }
}

void model_destroy(void) {
  db_cleanup();
}
