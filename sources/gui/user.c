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

#include "user.h"

enum {
  USER_PROP_AUTH = 1,
  USER_PROP_PID,
  USER_PROP_ACCESS_TOKEN,
  USER_PROP_FB_ACCESS_TOKEN,
  USER_LAST_PROPERTY
};

struct _User {
  GObject parent;
  int auth;
  char pid[0x100];
  char access_token[0x100];
  char fb_access_token[0x100];
};

static GParamSpec *user_properties[USER_LAST_PROPERTY] = { NULL, };

G_DEFINE_TYPE(User, user, G_TYPE_OBJECT)

static void user_init(User *obj) { }

static void user_get_property(GObject *object, guint property_id, GValue
    *value, GParamSpec *pspec) {
  User *u = (User *)object;

  switch (property_id) {
    case USER_PROP_AUTH:
      g_value_set_boolean(value, u->auth);
      break;
    case USER_PROP_PID:
      g_value_set_string(value, u->pid);
      break;
    case USER_PROP_ACCESS_TOKEN:
      g_value_set_string(value, u->access_token);
      break;
    case USER_PROP_FB_ACCESS_TOKEN:
      g_value_set_string(value, u->fb_access_token);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void user_set_property(GObject *object, guint property_id, const
    GValue *value, GParamSpec *pspec) {
  User *u = (User *)object;

  switch (property_id) {
    case USER_PROP_AUTH:
      u->auth = g_value_get_boolean(value);
      break;
    case USER_PROP_PID:
      strcpy(&u->pid[0], g_value_get_string(value));
      break;
    case USER_PROP_ACCESS_TOKEN:
      strcpy(&u->access_token[0], g_value_get_string(value));
      break;
    case USER_PROP_FB_ACCESS_TOKEN:
      strcpy(&u->fb_access_token[0], g_value_get_string(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void user_finalize(GObject *obj) {
  G_OBJECT_CLASS(user_parent_class)->finalize(obj);
}

static void user_class_init(UserClass *class) {
  GObjectClass *object_class = G_OBJECT_CLASS(class);

  object_class->get_property = user_get_property;
  object_class->set_property = user_set_property;
  object_class->finalize = user_finalize;

  user_properties[USER_PROP_AUTH] = g_param_spec_boolean("auth",
      "auth", "auth", FALSE, G_PARAM_READWRITE);
  user_properties[USER_PROP_PID] = g_param_spec_string("pid", "pid",
      "pid", NULL, G_PARAM_READWRITE);
  user_properties[USER_PROP_ACCESS_TOKEN] = g_param_spec_string("access-token",
      "access-token", "access-token", NULL, G_PARAM_READWRITE);
  user_properties[USER_PROP_FB_ACCESS_TOKEN] =
    g_param_spec_string("fb-access-token", "fb-access-token", "fb-access-token",
        NULL, G_PARAM_READWRITE);

  g_object_class_install_properties(object_class, USER_LAST_PROPERTY,
      user_properties);
}
