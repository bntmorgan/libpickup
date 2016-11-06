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
#include <string.h>
#include <stdlib.h>

#include "db.h"
#include "log.h"
#include "io.h"

sqlite3 *db;

/**
 * SQL statements
 */
char sql_pragma_foreign_keys[] = "PRAGMA foreign_keys = ON";
char sql_delete_person[] = "DELETE FROM persons WHERE pid = ?";
char sql_delete_match[] = "DELETE FROM persons WHERE pid in "
  "(SELECT id_person FROM matches WHERE mid = ?)";
char sql_delete_message[] = "DELETE FROM messages WHERE id = ?";
char sql_insert_person[] =
  "INSERT INTO persons (pid, name, birth) VALUES (?, ?, ?)";
char sql_insert_rec[] =
  "INSERT INTO recs (pid, date) VALUES (?, ?)";
char sql_insert_match[] =
  "INSERT INTO matches (mid, date, id_person) VALUES (?, ?, ?)";
char sql_insert_message[] =
  "INSERT INTO messages (id, dir, message, date, id_match) VALUES "
  "(?, ?, ?, ?, ?)";
char sql_insert_image[] =
  "INSERT INTO images (id, url, main, id_person) VALUES "
  "(?, ?, ?, ?)";
char sql_insert_image_processed[] =
  "INSERT INTO images_processed (url, width, height, id_image) VALUES "
  "(?, ?, ?, ?)";
char sql_select_matches_persons[] =
  "SELECT m.mid, m.date, p.pid, p.name, p.birth FROM matches AS m "
  "LEFT JOIN persons AS p ON p.pid = m.id_person order by m.date ASC";
char sql_select_recs_persons[] =
  "SELECT r.pid, r.date, p.name, p.birth FROM recs AS r "
  "LEFT JOIN persons AS p ON p.pid = r.pid";
char sql_select_match_person[] =
  "SELECT m.mid, m.date, p.pid, p.name, p.birth FROM matches AS m "
  "LEFT JOIN persons AS p ON p.pid = m.id_person WHERE p.pid = ?";
char sql_select_rec_person[] =
  "SELECT r.date, p.pid, p.name, p.birth FROM recs AS r LEFT JOIN persons AS p "
  "ON p.pid = r.pid WHERE p.pid = ?";

char sql_count_images[] =
  "SELECT COUNT(*) AS count FROM images WHERE id_person = ?";
char sql_count_images_processed[] =
  "SELECT count(*) AS count FROM images_processed WHERE id_image = ?";
char sql_count_messages[] =
  "SELECT count(*) AS count FROM messages AS msg "
  "LEFT JOIN matches AS m ON msg.id_match = m.mid "
  "LEFT JOIN persons AS p ON m.id_person = p.pid where p.pid = ?";
char sql_select_images[] =
  "SELECT id, url, main FROM images WHERE id_person = ?";
char sql_select_images_processed[] =
  "SELECT url, width, height FROM images_processed WHERE id_image = ?";
char sql_select_messages[] =
  "SELECT msg.id, msg.dir, msg.message, msg.date FROM messages AS msg "
  "LEFT JOIN matches AS m ON msg.id_match = m.mid "
  "LEFT JOIN persons as p ON m.id_person = p.pid where p.pid = ? "
  "order by msg.date ASC";

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

int db_delete_message(const char *id) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  rc = sqlite3_prepare_v2(db, sql_delete_message, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare delete statment %s (%i): %s\n", sql_delete_message, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  // Bind the sql request parameter : the message id
  rc = sqlite3_bind_text(stmt, 1, id, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value in delete (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  // Execute the delete statement
  rc = sqlite3_step(stmt);
  if(SQLITE_DONE != rc) {
    ERROR("delete statement didn't return DONE (%i): %s\n", rc,
        sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  DEBUG("Message %s dropped\n", id);

  sqlite3_finalize(stmt);
  return 0;
}

int db_delete_match(const char *mid) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  rc = sqlite3_prepare_v2(db, sql_delete_match, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare delete statment %s (%i): %s\n", sql_delete_person, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  // Bind the sql request parameter : the person id
  rc = sqlite3_bind_text(stmt, 1, mid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value in delete (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  // Execute the delete statement
  rc = sqlite3_step(stmt);
  if(SQLITE_DONE != rc) {
    ERROR("delete statement didn't return DONE (%i): %s\n", rc,
        sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  DEBUG("Match %s and associate person dropped\n", mid);

  sqlite3_finalize(stmt);
  return 0;
}

int db_delete_person(const char *pid) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  rc = sqlite3_prepare_v2(db, sql_delete_person, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare delete statment %s (%i): %s\n", sql_delete_person, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  // Bind the sql request parameter : the person id
  rc = sqlite3_bind_text(stmt, 1, pid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value in delete (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  // Execute the delete statement
  rc = sqlite3_step(stmt);
  if(SQLITE_DONE != rc) {
    ERROR("delete statement didn't return DONE (%i): %s\n", rc,
        sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  DEBUG("Person %s dropped\n", pid);

  sqlite3_finalize(stmt);
  return 0;
}

int db_insert_message(const struct cinder_message *m, const char *mid) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  rc = sqlite3_prepare_v2(db, sql_insert_message, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert statment %s (%i): %s\n", sql_insert_message, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  // Bind the sql request parameters
  rc = sqlite3_bind_text(stmt, 1, m->id, strlen(m->id)
      + 1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  rc = sqlite3_bind_int(stmt, 2, m->dir);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  rc = sqlite3_bind_text(stmt, 3, m->message, strlen(m->message)
      + 1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  rc = sqlite3_bind_int(stmt, 4, m->date);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  rc = sqlite3_bind_text(stmt, 5, mid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  // Execute the statement
  rc = sqlite3_step(stmt);
  if(SQLITE_DONE != rc) {
    ERROR("Statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);

  DEBUG("Message %s inserted\n", m->id);

  return 0;
}

int db_insert_image_processed(const struct cinder_image_processed *img, const
    char *id_image) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  rc = sqlite3_prepare_v2(db, sql_insert_image_processed, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert statment %s (%i): %s\n",
        sql_insert_image_processed, rc, sqlite3_errmsg(db));
    return -1;
  }

  // Bind the sql request parameters
  rc = sqlite3_bind_text(stmt, 1, img->url, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  rc = sqlite3_bind_int(stmt, 2, img->width);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  rc = sqlite3_bind_int(stmt, 3, img->height);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  rc = sqlite3_bind_text(stmt, 4, id_image, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  // Execute the statement
  rc = sqlite3_step(stmt);
  if(SQLITE_DONE != rc) {
    ERROR("Statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);

  DEBUG("Image processed %s inserted\n", id_image);

  return 0;
}

int db_insert_image(const struct cinder_image *img, const char *pid) {
  int i;
  int rc;
  sqlite3_stmt *stmt = NULL;

  rc = sqlite3_prepare_v2(db, sql_insert_image, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert statment %s (%i): %s\n", sql_insert_image, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  // Bind the sql request parameters
  rc = sqlite3_bind_text(stmt, 1, img->id, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  rc = sqlite3_bind_text(stmt, 2, img->url, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  rc = sqlite3_bind_int(stmt, 3, img->main);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  rc = sqlite3_bind_text(stmt, 4, pid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  // Execute the statement
  rc = sqlite3_step(stmt);
  if(SQLITE_DONE != rc) {
    ERROR("Statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);

  // Insert images processed
  for (i = 0; i < CINDER_SIZE_PROCESSED; i++) {
    if (db_insert_image_processed(&img->processed[i], img->id)) {
      ERROR("Failed to insert image processed %d of image %s\n", i, img->id);
      return -1;
    }
  }

  DEBUG("Image %s inserted\n", img->id);

  return 0;
}

int db_insert_person(const struct cinder_match *m) {
  int rc;
  int i;
  sqlite3_stmt *stmt = NULL;

  rc = sqlite3_prepare_v2(db, sql_insert_person, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert statment %s (%i): %s\n", sql_insert_person, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  // Bind the sql request parameters
  rc = sqlite3_bind_text(stmt, 1, m->pid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  rc = sqlite3_bind_text(stmt, 2, m->name, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }
  rc = sqlite3_bind_int(stmt, 3, m->birth);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  // Execute the statement
  rc = sqlite3_step(stmt);
  if(SQLITE_DONE != rc) {
    ERROR("Statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);

  // Insert images
  for (i = 0; i < m->images_count; i++) {
    if (db_insert_image(&m->images[i], m->pid) != 0) {
      ERROR("Failed to insert image %s\n", m->images[i].id);
      db_delete_person(m->pid);
      return -1;
    }
  }

  DEBUG("person %s inserted\n", m->pid);

  return 0;
}

int db_insert_match(const struct cinder_match *m) {
  int rc;
  int i;
  sqlite3_stmt *stmt = NULL;

  // The we create the person
  if (db_insert_person(m) != 0) {
    ERROR("Failed to insert person %s\n", m->pid);
    return -1;
  }

  // We can add the match
  rc = sqlite3_prepare_v2(db, sql_insert_match, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert statment %s (%i): %s\n", sql_insert_match, rc,
        sqlite3_errmsg(db));
    db_delete_person(m->pid);
    return -1;
  }

  // Bind the sql request parameters
  rc = sqlite3_bind_text(stmt, 1, m->mid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    db_delete_person(m->pid);
    return -1;
  }
  rc = sqlite3_bind_int(stmt, 2, m->date);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    db_delete_person(m->pid);
    return -1;
  }
  rc = sqlite3_bind_text(stmt, 3, m->pid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    db_delete_person(m->pid);
    return -1;
  }

  // Execute the statement
  rc = sqlite3_step(stmt);
  if(SQLITE_DONE != rc) {
    ERROR("Statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    db_delete_person(m->pid);
    return -1;
  }

  sqlite3_finalize(stmt);

  // Insert messages
  for (i = 0; i < m->messages_count; i++) {
    if (db_insert_message(&m->messages[i], m->mid) != 0) {
      ERROR ("Failed to insert message %s\n", m->messages[i].id);
      db_delete_person(m->pid);
      return -1;
    }
  }

  DEBUG("Match for person %s inserted\n", m->pid);

  return 0;
}

int db_update_message(const struct cinder_message *m, const char *mid) {
  int rc;

  // First drop the old message
  rc = db_delete_message(m->id);
  if (rc) {
    return -1;
  }

  // Insert everything new
  return db_insert_message(m, mid);
}

int db_update_match(const struct cinder_match *m) {
  int rc;

  // First drop the old match
  rc = db_delete_person(m->pid);
  if (rc) {
    return -1;
  }

  // Insert everything new
  return db_insert_match(m);
}

int db_select_matches(void (*cb_match)(struct cinder_match *)) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  rc = sqlite3_prepare_v2(db, sql_select_matches_persons, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert statment %s (%i): %s\n",
        sql_select_matches_persons, rc, sqlite3_errmsg(db));
    return -1;
  }

  rc = sqlite3_step(stmt);
  while (rc == SQLITE_ROW) {
    struct cinder_match m;
    int col;
    for(col=0; col < sqlite3_column_count(stmt); col++) {
      const char *col_name = sqlite3_column_name(stmt, col);
      const char *col_data = (char *)sqlite3_column_text(stmt, col);
      DEBUG("\tColumn %s(%i): '%s'\n", col_name, col, col_data);
      if (strcmp("mid", col_name) == 0) {
        strcpy(&m.mid[0], col_data);
      } else if (strcmp("date", col_name) == 0) {
        m.date = atoi(col_data);
      } else if (strcmp("pid", col_name) == 0) {
        strcpy(&m.pid[0], col_data);
      } else if (strcmp("name", col_name) == 0) {
        strcpy(&m.name[0], col_data);
      } else if (strcmp("birth", col_name) == 0) {
        m.birth= atoi(col_data);
      }
    }
    // Call the callback
    if (cb_match != NULL) {
      cb_match(&m);
    }
    rc = sqlite3_step(stmt);
  }
  if(SQLITE_DONE != rc) {
    ERROR("Statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);

  return 0;
}

int db_insert_rec(const struct cinder_match *m) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  // First we create the person
  if (db_insert_person(m) != 0) {
    ERROR("Failed to insert person %s\n", m->pid);
    return -1;
  }

  // We can add the match
  rc = sqlite3_prepare_v2(db, sql_insert_rec, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare statment %s (%i): %s\n", sql_insert_rec, rc,
        sqlite3_errmsg(db));
    db_delete_person(m->pid);
    return -1;
  }

  // Bind the sql request parameters
  rc = sqlite3_bind_text(stmt, 1, m->pid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    db_delete_person(m->pid);
    return -1;
  }
  rc = sqlite3_bind_int(stmt, 2, m->date);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    db_delete_person(m->pid);
    return -1;
  }

  // Execute the statement
  rc = sqlite3_step(stmt);
  if(SQLITE_DONE != rc) {
    ERROR("Statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    db_delete_person(m->pid);
    return -1;
  }

  DEBUG("Rec for person %s inserted\n", m->pid);

  sqlite3_finalize(stmt);

  return 0;
}

int db_update_rec(const struct cinder_match *m) {
  int rc;

  // First drop the old rec
  rc = db_delete_person(m->pid);
  if (rc) {
    return -1;
  }

  // Insert everything new
  return db_insert_rec(m);
}

int db_select_recs(void (*cb_recs)(struct cinder_match *)) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  rc = sqlite3_prepare_v2(db, sql_select_recs_persons, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert %s (%i): %s\n", sql_select_recs_persons, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  rc = sqlite3_step(stmt);
  while (rc == SQLITE_ROW) {
    struct cinder_match m;
    int col;
    for(col=0; col < sqlite3_column_count(stmt); col++) {
      const char *col_name = sqlite3_column_name(stmt, col);
      const char *col_data = (char *)sqlite3_column_text(stmt, col);
      DEBUG("\tColumn %s(%i): '%s'\n", col_name, col, col_data);
      if (strcmp("pid", col_name) == 0) {
        strcpy(&m.pid[0], col_data);
      } else if (strcmp("date", col_name) == 0) {
        m.date = atoi(col_data);
      } else if (strcmp("name", col_name) == 0) {
        strcpy(&m.name[0], col_data);
      } else if (strcmp("birth", col_name) == 0) {
        m.birth= atoi(col_data);
      }
    }
    // Call the callback
    if (cb_recs!= NULL) {
      cb_recs(&m);
    }
    rc = sqlite3_step(stmt);
  }
  if(SQLITE_DONE != rc) {
    ERROR("Statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);

  return 0;
}

int db_count_images_processed(const char *id_image, unsigned int *count) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  rc = sqlite3_prepare_v2(db, sql_count_images_processed, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert %s (%i): %s\n", sql_count_images_processed, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  rc = sqlite3_bind_text(stmt, 1, id_image, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  rc = sqlite3_step(stmt);

  if (rc != SQLITE_ROW) {
    ERROR("Statement didn't return ROW (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  if (sqlite3_column_count(stmt) != 1) {
    ERROR("Statement didn't return one row (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  const char *col_name = sqlite3_column_name(stmt, 0);
  const char *col_data = (char *)sqlite3_column_text(stmt, 0);

  if (strcmp(col_name, "count") != 0) {
    ERROR("Statement didn't return one row (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  *count = atoi(col_data);

  sqlite3_finalize(stmt);

  return 0;
}

int db_count_messages(const char *pid, unsigned int *count) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  rc = sqlite3_prepare_v2(db, sql_count_messages, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert %s (%i): %s\n", sql_count_messages, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  rc = sqlite3_bind_text(stmt, 1, pid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  rc = sqlite3_step(stmt);

  if (rc != SQLITE_ROW) {
    ERROR("Statement didn't return ROW (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  if (sqlite3_column_count(stmt) != 1) {
    ERROR("Statement didn't return one row (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  const char *col_name = sqlite3_column_name(stmt, 0);
  const char *col_data = (char *)sqlite3_column_text(stmt, 0);

  if (strcmp(col_name, "count") != 0) {
    ERROR("Statement didn't return one row (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  *count = atoi(col_data);

  sqlite3_finalize(stmt);

  return 0;
}

int db_count_images(const char *pid, unsigned int *count) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  rc = sqlite3_prepare_v2(db, sql_count_images, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert %s (%i): %s\n", sql_count_images, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  rc = sqlite3_bind_text(stmt, 1, pid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  rc = sqlite3_step(stmt);

  if (rc != SQLITE_ROW) {
    ERROR("Statement didn't return ROW (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  if (sqlite3_column_count(stmt) != 1) {
    ERROR("Statement didn't return one row (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  const char *col_name = sqlite3_column_name(stmt, 0);
  const char *col_data = (char *)sqlite3_column_text(stmt, 0);

  if (strcmp(col_name, "count") != 0) {
    ERROR("Statement didn't return one row (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  *count = atoi(col_data);

  sqlite3_finalize(stmt);

  return 0;
}

int db_select_images_processed(const char *id_image, struct cinder_image *img) {
  int rc;
  int i;
  unsigned int images_count;
  sqlite3_stmt *stmt = NULL;

  if (db_count_images_processed(id_image, &images_count) != 0) {
    ERROR("Failed to count images od person %s\n", id_image);
    return -1;
  }

  if (images_count != CINDER_SIZE_PROCESSED) {
    return -1;
  }

  rc = sqlite3_prepare_v2(db, sql_select_images_processed, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert %s (%i): %s\n", sql_select_images_processed, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  rc = sqlite3_bind_text(stmt, 1, id_image, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  i = 0;
  rc = sqlite3_step(stmt);
  while (rc == SQLITE_ROW) {
    struct cinder_image_processed *imgp = &img->processed[i];
    int col;
    for(col=0; col < sqlite3_column_count(stmt); col++) {
      const char *col_name = sqlite3_column_name(stmt, col);
      const char *col_data = (char *)sqlite3_column_text(stmt, col);
      DEBUG("\tColumn %s(%i): '%s'\n", col_name, col, col_data);
      if (strcmp("url", col_name) == 0) {
        strcpy(&imgp->url[0], col_data);
      } else if (strcmp("width", col_name) == 0) {
        imgp->width = atoi(col_data);
      } else if (strcmp("height", col_name) == 0) {
        imgp->height = atoi(col_data);
      }
    }
    rc = sqlite3_step(stmt);
    i++;
  }
  if(SQLITE_DONE != rc) {
    ERROR("Statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);

  return 0;
}

int db_select_images(const char *pid, struct cinder_match *m) {
  int rc;
  int i;
  sqlite3_stmt *stmt = NULL;

  if (db_count_images(pid, &m->images_count) != 0) {
    ERROR("Failed to count images od person %s\n", pid);
    return -1;
  }

  m->images = malloc(m->images_count * sizeof(struct cinder_image));
  if (m->images == NULL) {
    ERROR("Failed to allocate memory\n");
    return -1;
  }
  memset(m->images, 0, m->images_count * sizeof(struct cinder_image));

  rc = sqlite3_prepare_v2(db, sql_select_images, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert %s (%i): %s\n", sql_select_images, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  rc = sqlite3_bind_text(stmt, 1, pid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  i = 0;
  rc = sqlite3_step(stmt);
  while (rc == SQLITE_ROW) {
    struct cinder_image *img = &m->images[i];
    int col;
    for(col=0; col < sqlite3_column_count(stmt); col++) {
      const char *col_name = sqlite3_column_name(stmt, col);
      const char *col_data = (char *)sqlite3_column_text(stmt, col);
      DEBUG("\tColumn %s(%i): '%s'\n", col_name, col, col_data);
      if (strcmp("id", col_name) == 0) {
        strcpy(&img->id[0], col_data);
      } else if (strcmp("url", col_name) == 0) {
        strcpy(&img->url[0], col_data);
      } else if (strcmp("main", col_name) == 0) {
        img->main = atoi(col_data);
      }
    }
    if (db_select_images_processed(img->id, img) != 0) {
      ERROR("Error selecting the images processed associated to image %s\n",
          img->id);
      return -1;
    }
    rc = sqlite3_step(stmt);
    i++;
  }
  if(SQLITE_DONE != rc) {
    ERROR("Statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);

  return 0;
}

int db_select_messages(const char *pid, struct cinder_match *m) {
  int rc;
  int i;
  sqlite3_stmt *stmt = NULL;

  if (db_count_messages(pid, &m->messages_count) != 0) {
    ERROR("Failed to count messages od person %s\n", pid);
    return -1;
  }

  m->messages = malloc(m->messages_count * sizeof(struct cinder_message));
  if (m->messages == NULL) {
    ERROR("Failed to allocate memory\n");
    return -1;
  }
  memset(m->messages, 0, m->messages_count * sizeof(struct cinder_message));

  rc = sqlite3_prepare_v2(db, sql_select_messages, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert %s (%i): %s\n", sql_select_messages, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  rc = sqlite3_bind_text(stmt, 1, pid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  i = 0;
  rc = sqlite3_step(stmt);
  while (rc == SQLITE_ROW) {
    struct cinder_message *msg = &m->messages[i];
    int col;
    for(col=0; col < sqlite3_column_count(stmt); col++) {
      const char *col_name = sqlite3_column_name(stmt, col);
      const char *col_data = (char *)sqlite3_column_text(stmt, col);
      DEBUG("\tColumn %s(%i): '%s'\n", col_name, col, col_data);
      if (strcmp("id", col_name) == 0) {
        strcpy(&msg->id[0], col_data);
      } else if (strcmp("dir", col_name) == 0) {
        msg->dir = atoi(col_data);
      } else if (strcmp("message", col_name) == 0) {
        strcpy(&msg->message[0], col_data);
      } else if (strcmp("date", col_name) == 0) {
        msg->date = atoi(col_data);
      }
    }
    rc = sqlite3_step(stmt);
    i++;
  }
  if(SQLITE_DONE != rc) {
    ERROR("Statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  sqlite3_finalize(stmt);

  return 0;
}

int db_select_rec(const char *pid, struct cinder_match **match) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  struct cinder_match *m = malloc(sizeof(struct cinder_match));
  if (m == NULL) {
    ERROR("Failed to allocate memory\n");
    return -1;
  }
  memset(m, 0, sizeof(struct cinder_match));
  *match = m;

  rc = sqlite3_prepare_v2(db, sql_select_rec_person, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare %s (%i): %s\n", sql_select_recs_persons, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  rc = sqlite3_bind_text(stmt, 1, pid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    int col;
    for(col=0; col < sqlite3_column_count(stmt); col++) {
      const char *col_name = sqlite3_column_name(stmt, col);
      const char *col_data = (char *)sqlite3_column_text(stmt, col);
      DEBUG("\tColumn %s(%i): '%s'\n", col_name, col, col_data);
      if (strcmp("pid", col_name) == 0) {
        strcpy(&m->pid[0], col_data);
      } else if (strcmp("date", col_name) == 0) {
        m->date = atoi(col_data);
      } else if (strcmp("name", col_name) == 0) {
        strcpy(&m->name[0], col_data);
      } else if (strcmp("birth", col_name) == 0) {
        m->birth= atoi(col_data);
      }
    }
  } else {
    ERROR("Statement didn't return ROW (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  if (db_select_images(m->pid, m) != 0) {
    ERROR("Error selecting the images associated to person %s\n", m->pid);
    cinder_match_free(m);
    return -1;
  }

  if (db_select_messages(m->pid, m) != 0) {
    ERROR("Error selecting the images associated to person %s\n", m->pid);
    cinder_match_free(m);
    return -1;
  }

  sqlite3_finalize(stmt);

  return 0;
}

int db_select_match(const char *pid, struct cinder_match **match) {
  int rc;
  sqlite3_stmt *stmt = NULL;

  struct cinder_match *m = malloc(sizeof(struct cinder_match));
  if (m == NULL) {
    ERROR("Failed to allocate memory\n");
    return -1;
  }
  memset(m, 0, sizeof(struct cinder_match));
  *match = m;

  rc = sqlite3_prepare_v2(db, sql_select_match_person, -1, &stmt, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Can't prepare insert %s (%i): %s\n", sql_select_recs_persons, rc,
        sqlite3_errmsg(db));
    return -1;
  }

  rc = sqlite3_bind_text(stmt, 1, pid, -1, NULL);
  if(SQLITE_OK != rc) {
    ERROR("Error binding value (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    int col;
    for(col=0; col < sqlite3_column_count(stmt); col++) {
      const char *col_name = sqlite3_column_name(stmt, col);
      const char *col_data = (char *)sqlite3_column_text(stmt, col);
      DEBUG("\tColumn %s(%i): '%s'\n", col_name, col, col_data);
      if (strcmp("mid", col_name) == 0) {
        strcpy(&m->mid[0], col_data);
      } else if (strcmp("pid", col_name) == 0) {
        strcpy(&m->pid[0], col_data);
      } else if (strcmp("date", col_name) == 0) {
        m->date = atoi(col_data);
      } else if (strcmp("name", col_name) == 0) {
        strcpy(&m->name[0], col_data);
      } else if (strcmp("birth", col_name) == 0) {
        m->birth= atoi(col_data);
      }
    }
  } else {
    ERROR("Statement didn't return ROW (%i): %s\n", rc, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return -1;
  }

  if (db_select_images(m->pid, m) != 0) {
    ERROR("Error selecting the images associated to person %s\n", m->pid);
    cinder_match_free(m);
    return -1;
  }

  if (db_select_messages(m->pid, m) != 0) {
    ERROR("Error selecting the images associated to person %s\n", m->pid);
    cinder_match_free(m);
    return -1;
  }

  sqlite3_finalize(stmt);

  return 0;
}
