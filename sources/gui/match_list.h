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

#ifndef __MATCH_LIST_H__
#define __MATCH_LIST_H__

#include <glib-object.h>

#define MATCH_LIST_TYPE (match_list_get_type())
G_DECLARE_FINAL_TYPE(MatchList, match_list, PICKUP_GUI, MATCH_LIST,
    GObject)

MatchList *match_list_new(void);

#endif//__MATCH_LIST_H__
