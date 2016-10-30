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

#ifndef __IO_H__
#define __IO_H__

/**
 * Read or write a single string into a file
 */
int str_write(char *filename, const char *buf);
int str_read(char *filename, char *buf, size_t count);

/**
 * Manage the configuration files
 */
int file_unlink(char *filename);

#define IO_CONFIG_DIR ".config/cinder"

#endif//__IO_H__
