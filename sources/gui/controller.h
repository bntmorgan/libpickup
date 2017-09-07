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

#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

void controller_init(void);
void controller_cleanup(void);
void controller_set_match(const char *pid, unsigned int index);
void controller_set_rec(const char *pid, unsigned int index);
void controller_image_skip(int skip);
int controller_swipe_rec(int like);
int controller_recs_scan(void);
void controller_lock(int lock);
void controller_clear_match(void);

#endif//__CONTROLLER_H__
