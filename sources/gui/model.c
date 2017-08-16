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

#include <glib.h>
#include "db.h"
#include "log.h"

static GHashTable *matches;
static GHashTable *recs;

static void cb_match(struct pickup_match *m) {
  DEBUG("Selected mid(%s)\n", m->mid);
  // Add the match to the model
  g_hash_table_insert(matches, m->mid, m);
  // Update Gui
}

void model_init(void) {
  db_init();
  matches = g_hash_table_new(NULL, g_str_equal);
  recs = g_hash_table_new(NULL, g_str_equal);
}

void model_populate(void) {
  // Get all the recorded information
  if (db_select_matches(&cb_match)) {
    ERROR("Failed to access to db to get recorded matches\n");
  }
}

void model_destroy(void) {
  db_cleanup();
  g_hash_table_destroy(matches);
  g_hash_table_destroy(recs);
}
