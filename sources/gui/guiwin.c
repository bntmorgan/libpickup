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

#include <gtk/gtk.h>

#include "log.h"

#include "model.h"

#include "gui.h"
#include "guiwin.h"

struct _PickupAppWindow {
  GtkApplicationWindow parent;
};

typedef struct _PickupAppWindowPrivate PickupAppWindowPrivate;

struct _PickupAppWindowPrivate {
  GtkWidget *stack;
  GtkWidget *matches;
  GtkWidget *recs;
};

G_DEFINE_TYPE_WITH_PRIVATE(PickupAppWindow, pickup_app_window,
    GTK_TYPE_APPLICATION_WINDOW);

static GtkWidget *create_widget_match_list(gpointer item, gpointer user_data) {
  MatchList *obj = (MatchList *)item;
  GtkWidget *label;

  label = gtk_label_new("");
  g_object_bind_property(obj, "name", label, "label", G_BINDING_SYNC_CREATE);

  DEBUG("Label created for %s\n", obj->m.name);

  return label;
}

static void pickup_app_window_init(PickupAppWindow *app) {
  PickupAppWindowPrivate *priv;

  gtk_widget_init_template(GTK_WIDGET(app));

  priv = pickup_app_window_get_instance_private(app);

  gtk_list_box_bind_model(GTK_LIST_BOX(priv->matches), G_LIST_MODEL(matches),
      create_widget_match_list, NULL, NULL);
}

static void pickup_app_window_class_init(PickupAppWindowClass *class) {
  // Associate the windowtemplate
  gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
      "/org/gtk/gui/sources/gui/window.ui");

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, stack);

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, matches);

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, recs);
}

PickupAppWindow *pickup_app_window_new (PickupApp *app) {
  return g_object_new(PICKUP_APP_WINDOW_TYPE, "application", app, NULL);
}
