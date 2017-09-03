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

#include "message.h"

enum {
  MESSAGE_PROP_ID = 1,
  MESSAGE_PROP_DIRECTION,
  MESSAGE_PROP_MESSAGE,
  MESSAGE_PROP_DATE,
  MESSAGE_LAST_PROPERTY
};

struct _Message {
  GObject parent;
  struct pickup_message m;
};

static GParamSpec *message_properties[MESSAGE_LAST_PROPERTY] = { NULL, };

G_DEFINE_TYPE(Message, message, G_TYPE_OBJECT)

static void message_init(Message *obj) { }

static void message_get_property(GObject *object, guint property_id, GValue
    *value, GParamSpec *pspec) {
  struct pickup_message *m = &((Message *)object)->m;

  switch (property_id) {
    case MESSAGE_PROP_ID:
      g_value_set_string(value, m->id);
      break;
    case MESSAGE_PROP_MESSAGE:
      g_value_set_string(value, m->message);
      break;
    case MESSAGE_PROP_DATE:
      g_value_set_int(value, m->date);
      break;
    case MESSAGE_PROP_DIRECTION:
      g_value_set_int(value, m->dir);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void message_set_property(GObject *object, guint property_id, const
    GValue *value, GParamSpec *pspec) {
  struct pickup_message *m = &((Message *)object)->m;

  switch (property_id) {
    case MESSAGE_PROP_ID:
      strcpy(&m->id[0], g_value_get_string(value));
      break;
    case MESSAGE_PROP_MESSAGE:
      strcpy(&m->message[0], g_value_get_string(value));
      break;
    case MESSAGE_PROP_DATE:
      m->date = g_value_get_int(value);
      break;
    case MESSAGE_PROP_DIRECTION:
      m->dir = g_value_get_int(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void message_finalize(GObject *obj) {
  G_OBJECT_CLASS(message_parent_class)->finalize(obj);
}

static void message_class_init(MessageClass *class) {
  GObjectClass *object_class = G_OBJECT_CLASS(class);

  object_class->get_property = message_get_property;
  object_class->set_property = message_set_property;
  object_class->finalize = message_finalize;

  message_properties[MESSAGE_PROP_ID] = g_param_spec_string("id", "id",
      "id", NULL, G_PARAM_READWRITE);
  message_properties[MESSAGE_PROP_MESSAGE] = g_param_spec_string("message",
      "message", "message", NULL, G_PARAM_READWRITE);
  message_properties[MESSAGE_PROP_DATE] = g_param_spec_int("date", "date",
      "date", 0, G_MAXINT, 0, G_PARAM_READWRITE);
  message_properties[MESSAGE_PROP_DIRECTION] = g_param_spec_int("dir",
      "dir", "dir", 0, G_MAXINT, 0, G_PARAM_READWRITE);

  g_object_class_install_properties(object_class, MESSAGE_LAST_PROPERTY,
      message_properties);
}
