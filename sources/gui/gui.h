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

#ifndef __EXAMPLEAPP_H
#define __EXAMPLEAPP_H

#include <gtk/gtk.h>


#define EXAMPLE_APP_TYPE (example_app_get_type ())
G_DECLARE_FINAL_TYPE (ExampleApp, example_app, EXAMPLE, APP, GtkApplication)

ExampleApp     *example_app_new         (void);


#endif /* __EXAMPLEAPP_H */
