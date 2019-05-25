/*
 * This file is part of vbig.
 * Copyright (C) 2019 Richard Kettlewell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef VBIG_H
#define VBIG_H

#include <config.h>
#include <string>

void capture(std::string &output, const char *file, const char **args);
bool safe_path(const std::string &path);
bool is_block_device(const std::string &path);
bool block_device_in_use(const std::string &path);
void __attribute__((noreturn)) fatal(int errno_value, const char *fmt, ...);

#endif /* VBIG_H */
