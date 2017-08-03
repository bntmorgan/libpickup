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

struct _ExampleAppWindow
{
  GtkApplicationWindow parent;
};

typedef struct _ExampleAppWindowPrivate ExampleAppWindowPrivate;

struct _ExampleAppWindowPrivate
{
  GtkWidget *stack;
};

G_DEFINE_TYPE_WITH_PRIVATE(ExampleAppWindow, example_app_window,
    GTK_TYPE_APPLICATION_WINDOW);

static void
example_app_window_init (ExampleAppWindow *app)
{
  gtk_widget_init_template (GTK_WIDGET (app));
}

static void
example_app_window_class_init (ExampleAppWindowClass *class)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
      "/org/gtk/gui/sources/gui/window.ui");

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
      ExampleAppWindow, stack);
}

ExampleAppWindow *
example_app_window_new (ExampleApp *app)
{
  return g_object_new (EXAMPLE_APP_WINDOW_TYPE, "application", app, NULL);
}

void
example_app_window_open (ExampleAppWindow *win,
                         GFile            *file)
{
  ExampleAppWindowPrivate *priv;
  gchar *basename;
  GtkWidget *scrolled, *view;
  gchar *contents;
  gsize length;

  priv = example_app_window_get_instance_private (win);
  basename = g_file_get_basename (file);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolled);
  gtk_widget_set_hexpand (scrolled, TRUE);
  gtk_widget_set_vexpand (scrolled, TRUE);
  view = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
  gtk_widget_show (view);
  gtk_container_add (GTK_CONTAINER (scrolled), view);
  gtk_stack_add_titled (GTK_STACK (priv->stack), scrolled, basename, basename);

  if (g_file_load_contents (file, NULL, &contents, &length, NULL, NULL))
  {
    GtkTextBuffer *buffer;

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
    gtk_text_buffer_set_text (buffer, contents, length);
    g_free (contents);
  }

  g_free (basename);
}
