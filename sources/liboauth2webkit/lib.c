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

#include <stdlib.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <string.h>
#include <pcre.h>

#include "oauth2webkit/oauth2webkit.h"

struct context {
  char *access_token;
  const char *url_confirm;
  int error_code;
};

static void destroy_window(GtkWidget* widget, GtkWidget* window, gpointer
    user_data);
static void resource_load_finished(WebKitWebView *web_view, WebKitWebFrame
    *web_frame, WebKitWebResource *web_resource, gpointer user_data);

static int *oauth2_argc = NULL;
static char ***oauth2_argv = NULL;

void oauth2_init(int *argc, char ***argv) {
  oauth2_argc = argc;
  oauth2_argv = argv;
}

int oauth2_get_access_token(const char *url, const char *url_confirm, char
    *access_token) {
  struct context ctx;

  if (url ==  NULL || access_token == NULL) {
    return 1;
  }

  // Initialize context
  memset(&ctx, 0, sizeof(struct context));
  ctx.error_code = OAUTH2_OK;
  ctx.access_token = access_token;
  ctx.url_confirm = url_confirm;

  // Initialize GTK+
  gtk_init(oauth2_argc, oauth2_argv);

  // Create an 800x600 window that will contain the browser instance
  GtkWidget *main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 600);

  // Create a browser instance
  WebKitWebView *webView = WEBKIT_WEB_VIEW(webkit_web_view_new());

  // Put the browser area into the main window
  gtk_container_add(GTK_CONTAINER(main_window), GTK_WIDGET(webView));

  // Set up callbacks so that if either the main window or the browser instance
  // is closed, the program will exit
  g_signal_connect(main_window, "destroy", G_CALLBACK(destroy_window), NULL);
  // g_signal_connect(main_window, "closed", G_CALLBACK(destroy_window), NULL);

  g_signal_connect(webView, "resource-load-finished",
      G_CALLBACK(resource_load_finished), &ctx);

  // Load a web page into the browser instance
  webkit_web_view_load_uri(webView, url);

  // Make sure that when the browser area becomes visible, it will get mouse
  // and keyboard events
  gtk_widget_grab_focus(GTK_WIDGET(webView));

  // Make sure the main window and all its contents are visible
  gtk_widget_show_all(main_window);

  // Run the main GTK+ event loop
  gtk_main();

  return ctx.error_code;
}

static void destroy_window(GtkWidget* widget, GtkWidget* window, gpointer
    user_data) {
  struct context *ctx = (struct context *)user_data;
  ctx->error_code = OAUTH2_USER_CLOSED;
  gtk_main_quit();
}

static int parse_result(const char *data, char *access_token) {
  pcre *re_comp;
  char *re_reg = "access_token=([^&]*)";
  const char *re_err_str;
  int re_err_offset;
  pcre_extra *re_extra;
  int re_ret;
  int re_sub_str_ret[30] = {};

  re_comp = pcre_compile(re_reg, 0, &re_err_str, &re_err_offset, NULL);
  if (re_comp == NULL) {
    fprintf(stderr, "ERROR: Failed to build the regular expression %s: %s\n",
        re_reg, re_err_str);
    exit(1);
  }

  re_extra = pcre_study(re_comp, 0, &re_err_str);
  if (re_err_str != NULL) {
    fprintf(stderr, "ERROR: Could not study '%s': %s\n", re_reg, re_err_str);
    exit(1);
  }

  re_ret = pcre_exec(re_comp, re_extra, data, strlen(data), 0, 0,
      re_sub_str_ret, 30);

  if(re_ret < 0) {
    switch(re_ret) {
      case PCRE_ERROR_NOMATCH :
        fprintf(stderr, "String did not match the pattern\n");
        break;
      case PCRE_ERROR_NULL :
        fprintf(stderr, "Something was null\n");
        break;
      case PCRE_ERROR_BADOPTION :
        fprintf(stderr, "A bad option was passed\n");
        break;
      case PCRE_ERROR_BADMAGIC :
        fprintf(stderr, "Magic number bad (compiled re corrupt?)\n");
        break;
      case PCRE_ERROR_UNKNOWN_NODE :
        fprintf(stderr, "Something kooky in the compiled re\n");
        break;
      case PCRE_ERROR_NOMEMORY :
        fprintf(stderr, "Ran out of memory\n");
        break;
      default:
        fprintf(stderr, "Unknown error\n");
        break;
    }
    exit(1);
  } else {
    strncpy(access_token, &data[re_sub_str_ret[2]], re_sub_str_ret[3] -
        re_sub_str_ret[2]);
    access_token[re_sub_str_ret[3] - re_sub_str_ret[2]] = '\0';
    printf("access_token %s\n", access_token);
  }
  return 0;
}

static void resource_load_finished(WebKitWebView *web_view, WebKitWebFrame
    *web_frame, WebKitWebResource *web_resource, gpointer user_data) {
  const gchar *uri = webkit_web_resource_get_uri(web_resource);
  GString *data;
  struct context *ctx = (struct context *)user_data;

  if (strcmp(uri, ctx->url_confirm) == 0) {
    printf("resource uri : %s\n", uri);
    data = webkit_web_resource_get_data(web_resource);
    ctx->error_code = parse_result(data->str, ctx->access_token);
    gtk_main_quit();
  }
}