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

#ifndef __WORKER_H__
#define __WORKER_H__

typedef void *(*worker_thread_t) (void *data);
typedef void (*worker_idle_t) (void *data);

void worker_run(const char *name, worker_thread_t worker, void *data);
void worker_idle_add(worker_idle_t idle, void *data);

#endif//__WORKER_H__
