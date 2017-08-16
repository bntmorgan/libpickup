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

#ifndef __PICKUP_APP_WIN_H__
#define __PICKUP_APP_WIN_H__

#include <gtk/gtk.h>
#include "gui.h"

#define PICKUP_APP_WINDOW_TYPE (pickup_app_window_get_type())
G_DECLARE_FINAL_TYPE(PickupAppWindow, pickup_app_window, PICKUP, APP_WINDOW,
    GtkApplicationWindow)

PickupAppWindow *pickup_app_window_new(PickupApp *app);
void pickup_app_window_open(PickupAppWindow *win, GFile *file);

#endif /* __PICKUP_APP_WIN_H__ */
