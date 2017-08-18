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

#include <string.h>

#include <pickup/pickup.h>

#include "model.h"
#include "db.h"

void controller_init(void) {
  model_init();
  model_populate();
}

void controller_destroy(void) {
  model_destroy();
}

void controller_set_match(const char *pid) {
  struct pickup_match *m;
  db_select_match(pid, &m);
  pickup_match_free(m);
}

void controller_set_rec(const char *pid) {
  struct pickup_match *m;
  db_select_rec(pid, &m);
  pickup_match_free(m);
}
