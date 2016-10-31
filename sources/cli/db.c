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

#include <sqlite3.h>

#include "db.h"
#include "log.h"
#include "io.h"

sqlite3 *db;

/**
 * SQL statements
 */
char sql_pragma_foreign_keys[] = "PRAGMA foreign_keys = ON";
char sql_match_remove[] =
  "REMOVE FROM matches WHERE mid = ?";
char sql_match_insert[] =
  "INSERT INTO matches (mid, pid, name, birth) VALUES (?, ?, ?, ?)";
char sql_message_insert[] =
  "INSERT INTO messages (id, dir, message, date, id_match) VALUES "
  "(?, ?, ?, ?, ?)";
char sql_image_insert[] =
  "INSERT INTO images (id, url, filename, main, id_match) VALUES "
  "(?, ?, ?, ?, ?)";
char sql_image_processed_insert[] =
  "INSERT INTO images_processed (id, url, filename, main, id_match) VALUES "
  "(?, ?, ?, ?, ?)";

int db_init(void) {
  char db_path[0x100];
  int rc;
  char *errmsg;

  path_resolve(DB_FILENAME, &db_path[0], 0x100);
  DEBUG("DB path %s\n", db_path);

  if (sqlite3_open(db_path, &db) != SQLITE_OK) {
    ERROR("Error opening database\n");
    return -1;
  }

  // Use exec to run simple statements that can only fail/succeed
  rc = sqlite3_exec(db, sql_pragma_foreign_keys, NULL, NULL, &errmsg);
  if (SQLITE_OK != rc) {
    ERROR("Error creating table (%i): %s\n", rc, errmsg);
    sqlite3_free(errmsg);
    return -1;
  }

  return 0;
}

int db_cleanup(void) {
  if (sqlite3_close(db) != SQLITE_OK) {
    ERROR("Error closing database\n");
    return -1;
  }

  return 0;
}

int db_match_update(const struct cinder_match *m) {
  return 0;
}
