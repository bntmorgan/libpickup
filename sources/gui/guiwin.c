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
#include <gdk/gdkkeysyms-compat.h>

#include <pickup/pickup.h>

#include "log.h"

#include "match_list.h"
#include "match.h"
#include "message.h"
#include "model.h"
#include "controller.h"

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
  GtkWidget *match_name;
  GtkWidget *match_birth;
  GtkWidget *match_image;
  GtkWidget *match_pid;
  GtkWidget *next;
  GtkWidget *previous;
  GtkWidget *messages;
  GtkWidget *messages_panel;
  GtkWidget *like;
  GtkWidget *dislike;
};

G_DEFINE_TYPE_WITH_PRIVATE(PickupAppWindow, pickup_app_window,
    GTK_TYPE_APPLICATION_WINDOW);

static GtkWidget *create_widget_match_list(gpointer item, gpointer user_data) {
  MatchList *obj = (MatchList *)item;
  GtkWidget *label;
  gchar *name;

  label = gtk_label_new("");
  g_object_set(label, "xalign", 0., NULL);

  g_object_bind_property(obj, "name", label, "label", G_BINDING_SYNC_CREATE);

  g_object_get(obj, "name", &name, NULL);
  DEBUG("Label created for %s\n", name);

  return label;
}

static GtkWidget *create_widget_rec_list(gpointer item, gpointer user_data) {
  MatchList *obj = (MatchList *)item;
  GtkWidget *label;
  gchar *name;

  label = gtk_label_new("");
  g_object_set(label, "xalign", 0., NULL);

  g_object_bind_property(obj, "name", label, "label", G_BINDING_SYNC_CREATE);

  g_object_get(obj, "name", &name, NULL);
  DEBUG("Label created for %s\n", name);

  return label;
}

static GtkWidget *create_widget_message(gpointer item, gpointer user_data) {
  Message *obj = (Message *)item;
  GtkWidget *label;
  gchar *name;
  int dir;

  g_object_get(obj, "message", &name, "dir", &dir, NULL);

  label = gtk_entry_new();

  if (dir) {
    g_object_set(label, "xalign", 1., NULL);
  } else {
    g_object_set(label, "xalign", 0., NULL);
  }

  g_object_bind_property(obj, "message", label, "text", G_BINDING_SYNC_CREATE);

  g_object_set(label, "editable", FALSE, "has-frame", FALSE, NULL);

  DEBUG("Label created for %s\n", name);

  return label;
}

void matches_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer ms) {
  MatchList *m;
  gchar *name, *pid;
  m = (MatchList *)g_list_model_get_item((GListModel *)ms,
      gtk_list_box_row_get_index(row));
  g_object_get(m, "pid", &pid, "name", &name, NULL);
  DEBUG("Selected match %s[%s]\n", name, pid);
  controller_set_match((const char *)pid);
}

void recs_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer ms) {
  MatchList *m;
  gchar *name, *pid;
  m = (MatchList *)g_list_model_get_item((GListModel *)ms,
      gtk_list_box_row_get_index(row));
  g_object_get(m, "pid", &pid, "name", &name, NULL);
  DEBUG("Selected rec %s[%s]\n", name, pid);
  controller_set_rec((const char *)pid);
}

void next_clicked(GtkButton *button) {
  DEBUG("Next clicked\n");
  controller_image_skip(1);
}

void previous_clicked(GtkButton *button) {
  DEBUG("Previous clicked\n");
  controller_image_skip(-1);
}

gboolean key_press(GtkWidget *widget, GdkEventKey *event, gpointer data){
  switch(event->keyval) {
    case GDK_Right:
      DEBUG("Right pressed\n");
      controller_image_skip(1);
      return TRUE;
    case GDK_Left:
      DEBUG("Left pressed\n");
      controller_image_skip(-1);
      return TRUE;
  }
  return FALSE; // Propagate
}

static void pickup_app_window_init(PickupAppWindow *app) {
  PickupAppWindowPrivate *priv;

  gtk_widget_init_template(GTK_WIDGET(app));

  priv = pickup_app_window_get_instance_private(app);

  // Bind the models to the view

  gtk_list_box_bind_model(GTK_LIST_BOX(priv->matches), G_LIST_MODEL(matches),
      create_widget_match_list, NULL, NULL);

  gtk_list_box_bind_model(GTK_LIST_BOX(priv->recs), G_LIST_MODEL(recs),
      create_widget_rec_list, NULL, NULL);

  gtk_list_box_bind_model(GTK_LIST_BOX(priv->messages), G_LIST_MODEL(messages),
      create_widget_message, NULL, NULL);

  g_object_bind_property(selected, "name", priv->match_name, "text",
      G_BINDING_DEFAULT);

  g_object_bind_property(selected, "birth", priv->match_birth, "text",
      G_BINDING_DEFAULT);

  g_object_bind_property(selected, "image", priv->match_image, "file",
      G_BINDING_DEFAULT);

  g_object_bind_property(selected, "pid", priv->match_pid, "text",
      G_BINDING_DEFAULT);

  g_object_bind_property(selected, "match", priv->messages_panel, "visible",
      G_BINDING_SYNC_CREATE);

  g_object_bind_property(selected, "match", priv->like, "visible",
      G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  g_object_bind_property(selected, "match", priv->dislike, "visible",
      G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  // Connect the signals
  g_signal_connect(priv->matches, "row-selected",
      G_CALLBACK(matches_row_selected), G_LIST_MODEL(matches));

  g_signal_connect(priv->recs, "row-selected",
      G_CALLBACK(recs_row_selected), G_LIST_MODEL(recs));

  g_signal_connect(priv->next, "clicked", G_CALLBACK(next_clicked), NULL);

  g_signal_connect(priv->previous, "clicked", G_CALLBACK(previous_clicked),
      NULL);

  g_signal_connect(G_OBJECT(app), "key_press_event", G_CALLBACK(key_press),
      NULL);

  DEBUG("GListModel pointer %p\n", G_LIST_MODEL(matches));
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

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, match_name);

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, match_birth);

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, match_image);

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, match_pid);

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, next);

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, previous);

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, messages);

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, messages_panel);

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, like);

  gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class),
      PickupAppWindow, dislike);
}

PickupAppWindow *pickup_app_window_new (PickupApp *app) {
  return g_object_new(PICKUP_APP_WINDOW_TYPE, "application", app, NULL);
}
