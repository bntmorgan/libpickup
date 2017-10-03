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

#include <pickup/pickup.h>

#include "note.h"

enum {
  NOTE_PROP_TYPE = 1,
  NOTE_PROP_MESSAGE,
  NOTE_LAST_PROPERTY,
};

struct _Note {
  GObject parent;
  int type;
  char message[PICKUP_SIZE_MESSAGE];
};

static GParamSpec *note_properties[NOTE_LAST_PROPERTY] = { NULL, };

G_DEFINE_TYPE(Note, note, G_TYPE_OBJECT)

static void note_init(Note *obj) { }

static void note_get_property(GObject *object, guint property_id, GValue
    *value, GParamSpec *pspec) {
  Note *n = (Note *)object;

  switch (property_id) {
    case NOTE_PROP_TYPE:
      g_value_set_int(value, n->type);
      break;
    case NOTE_PROP_MESSAGE:
      g_value_set_string(value, n->message);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void note_set_property(GObject *object, guint property_id, const
    GValue *value, GParamSpec *pspec) {
  Note *n = (Note *)object;

  switch (property_id) {
    case NOTE_PROP_TYPE:
      n->type = g_value_get_int(value);
      break;
    case NOTE_PROP_MESSAGE:
      strcpy(&n->message[0], g_value_get_string(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void note_finalize(GObject *obj) {
  G_OBJECT_CLASS(note_parent_class)->finalize(obj);
}

static void note_class_init(NoteClass *class) {
  GObjectClass *object_class = G_OBJECT_CLASS(class);

  object_class->get_property = note_get_property;
  object_class->set_property = note_set_property;
  object_class->finalize = note_finalize;

  note_properties[NOTE_PROP_TYPE] = g_param_spec_int("type",
      "type", "type", 0, G_MAXINT, 0, G_PARAM_READWRITE);
  note_properties[NOTE_PROP_MESSAGE] = g_param_spec_string("message", "message",
      "message", NULL, G_PARAM_READWRITE);

  g_object_class_install_properties(object_class, NOTE_LAST_PROPERTY,
      note_properties);
}
