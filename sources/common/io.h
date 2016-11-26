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

#ifndef __IO_H__
#define __IO_H__

enum io_path_type {
  IO_PATH_CONFIG,
  IO_PATH_CACHE,
  IO_PATH_CACHE_IMG
};

/**
 * Resovle configuration files path
 */
int path_resolve(const char *filename, int type, char *path, size_t n);

/**
 * Read or write a single string into a file
 */
int str_write(char *filename, const char *buf);
int str_read(char *filename, char *buf, size_t count);
int file_write(char *filename, int type, char *buf, size_t count);
int file_exists(char *filename, int type);

/**
 * Manage the configuration files
 */
int file_unlink(char *filename, int type);

#define IO_CONFIG_DIR ".config/pickup"
#define IO_CACHE_DIR ".cache/pickup"
#define IO_CACHE_IMG_DIR ".cache/pickup/img"

#endif//__IO_H__
