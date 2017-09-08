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

#include "controller.h"

#include "log.h"

struct _PickupApp {
  GtkApplication parent;
};

G_DEFINE_TYPE(PickupApp, pickup_app, GTK_TYPE_APPLICATION);

static void pickup_app_init(PickupApp *app) { }

static void quit_activated(GSimpleAction *action, GVariant *parameter,
    gpointer app) {
  controller_cleanup();
  g_application_quit(G_APPLICATION(app));
}

static void recs_scan_activated(GSimpleAction *action, GVariant *parameter,
    gpointer app) {
  DEBUG("Scan clicked\n");
  controller_recs_scan();
}

static void updates_activated(GSimpleAction *action, GVariant *parameter,
    gpointer app) {
  DEBUG("Updates clicked\n");
  controller_updates();
}

static GActionEntry app_entries[] = {
  { "recs-scan", recs_scan_activated, NULL, NULL, NULL },
  { "updates", updates_activated, NULL, NULL, NULL },
  { "quit", quit_activated, NULL, NULL, NULL }
};

static void pickup_app_startup(GApplication *app) {
  GtkBuilder *builder;
  GMenuModel *app_menu;
  const gchar *quit_accels[2] = { "<Ctrl>Q", NULL };

  G_APPLICATION_CLASS(pickup_app_parent_class)->startup(app);

  g_action_map_add_action_entries(G_ACTION_MAP(app), app_entries, G_N_ELEMENTS
      (app_entries), app);
  gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.quit",
      quit_accels);

  builder =
    gtk_builder_new_from_resource("/org/gtk/gui/sources/gui/app-menu.ui");
  app_menu = G_MENU_MODEL(gtk_builder_get_object (builder, "appmenu"));
  gtk_application_set_app_menu(GTK_APPLICATION(app), app_menu);
  g_object_unref(builder);
}

static void pickup_app_activate(GApplication *app) {
  PickupAppWindow *win;

  win = pickup_app_window_new(PICKUP_APP(app));
  gtk_window_present(GTK_WINDOW(win));
}

static void pickup_app_class_init(PickupAppClass *class) {
  G_APPLICATION_CLASS(class)->startup = pickup_app_startup;
  G_APPLICATION_CLASS(class)->activate = pickup_app_activate;
}

PickupApp *pickup_app_new(void) {
  return g_object_new(PICKUP_APP_TYPE, "application-id", "org.gtk.gui",
      "flags", G_APPLICATION_HANDLES_OPEN, NULL);
}
