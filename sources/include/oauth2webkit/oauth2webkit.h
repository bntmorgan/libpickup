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

#ifndef __OAUTH2WEBKIT_H__
#define __OAUTH2WEBKIT_H__

enum oauth2_error_code {
  OAUTH2_OK,
  OAUTH2_REGEX,
  OAUTH2_USER_CLOSED
};

void oauth2_init(int *argc, char ***argv);
int oauth2_get_access_token(const char *url, char *access_token);

#endif//__OAUTH2WEBKIT_H__
