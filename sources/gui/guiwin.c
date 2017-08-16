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

#include "gui.h"
#include "guiwin.h"

struct _PickupAppWindow {
  GtkApplicationWindow parent;
};

typedef struct _PickupAppWindowPrivate PickupAppWindowPrivate;

struct _PickupAppWindowPrivate {
  GtkWidget *stack;
};

G_DEFINE_TYPE_WITH_PRIVATE(PickupAppWindow, pickup_app_window,
    GTK_TYPE_APPLICATION_WINDOW);

static void pickup_app_window_init(PickupAppWindow *app) {
  gtk_widget_init_template(GTK_WIDGET(app));
}

static void pickup_app_window_class_init(PickupAppWindowClass *class) {
  gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
      "/org/gtk/gui/sources/gui/window.ui");

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, stack);
}

PickupAppWindow *pickup_app_window_new (PickupApp *app) {
  return g_object_new(PICKUP_APP_WINDOW_TYPE, "application", app, NULL);
}
