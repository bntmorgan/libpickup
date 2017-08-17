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

#include "db.h"
#include "log.h"

// MatchList GTYPE wrapper

enum {
  MATCH_LIST_PROP_MID = 1,
  MATCH_LIST_PROP_PID,
  MATCH_LIST_PROP_NAME,
  MATCH_LIST_PROP_DATE,
  MATCH_LIST_PROP_BIRTH,
  MATCH_LIST_LAST_PROPERTY
};

static GParamSpec *properties[MATCH_LIST_LAST_PROPERTY] = { NULL, };

G_DEFINE_TYPE(MatchList, match_list, G_TYPE_OBJECT)

static void match_list_init(MatchList *obj) { }

static void match_list_get_property(GObject *object, guint property_id, GValue
    *value, GParamSpec *pspec) {
  struct pickup_match *m = &((MatchList *)object)->m;

  switch (property_id) {
    case MATCH_LIST_PROP_MID:
      g_value_set_string(value, m->mid);
      break;
    case MATCH_LIST_PROP_PID:
      g_value_set_string(value, m->pid);
      break;
    case MATCH_LIST_PROP_NAME:
      g_value_set_string(value, m->name);
      break;
    case MATCH_LIST_PROP_DATE:
      g_value_set_int(value, m->date);
      break;
    case MATCH_LIST_PROP_BIRTH:
      g_value_set_int(value, m->birth);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void match_list_set_property (GObject *object, guint property_id, const
    GValue *value, GParamSpec *pspec) {
  struct pickup_match *m = &((MatchList *)object)->m;

  switch (property_id) {
    case MATCH_LIST_PROP_MID:
      strcpy(&m->mid[0], g_value_get_string(value));
      break;
    case MATCH_LIST_PROP_PID:
      strcpy(&m->pid[0], g_value_get_string(value));
      break;
    case MATCH_LIST_PROP_NAME:
      strcpy(&m->name[0], g_value_get_string(value));
      break;
    case MATCH_LIST_PROP_DATE:
      m->date = g_value_get_int(value);
      break;
    case MATCH_LIST_PROP_BIRTH:
      m->birth = g_value_get_int(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void match_list_finalize (GObject *obj) {
  // MatchList *object = (MatchList *)obj;
  // TODO
  // g_free(object->label);
  G_OBJECT_CLASS(match_list_parent_class)->finalize(obj);
}

static void match_list_class_init(MatchListClass *class) {
  GObjectClass *object_class = G_OBJECT_CLASS(class);

  object_class->get_property = match_list_get_property;
  object_class->set_property = match_list_set_property;
  object_class->finalize = match_list_finalize;

  properties[MATCH_LIST_PROP_MID] = g_param_spec_string("mid", "mid", "mid",
      NULL, G_PARAM_READWRITE);
  properties[MATCH_LIST_PROP_PID] = g_param_spec_string("pid", "pid", "pid",
      NULL, G_PARAM_READWRITE);
  properties[MATCH_LIST_PROP_NAME] = g_param_spec_string("name", "name", "name",
      NULL, G_PARAM_READWRITE);
  properties[MATCH_LIST_PROP_DATE] = g_param_spec_int("date", "date", "date",
      0, G_MAXINT, 0, G_PARAM_READWRITE);
  properties[MATCH_LIST_PROP_BIRTH] = g_param_spec_int("birth", "birth",
      "birth", 0, G_MAXINT, 0, G_PARAM_READWRITE);

  g_object_class_install_properties(object_class, MATCH_LIST_LAST_PROPERTY,
      properties);
}

// END MatchList GTYPE wrapper

GListStore *matches;

static void cb_match(struct pickup_match *m) {
  DEBUG("Selected mid(%s)\n", m->mid);
  // Update model
  MatchList *obj;
  pickup_match_print(m);
  obj = g_object_new(match_list_get_type(), "mid", m->mid, "pid", m->pid,
      "name", m->name, "date", m->date, "birth", m->birth, NULL);
  g_list_store_append(matches, obj);
}

void model_init(void) {
  db_init();
  matches = g_list_store_new(match_list_get_type());
}

void model_populate(void) {
  // Get all the recorded information
  if (db_select_matches(&cb_match)) {
    ERROR("Failed to access to db to get recorded matches\n");
  }
}

void model_destroy(void) {
  db_cleanup();
}
