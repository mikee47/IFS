/**
 * xattr.h
 * Extended Attributes
 *
 * Created on: 16 Feb 2021
 *
 * Copyright 2019 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the IFS Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ****/

#pragma once

#include <cstddef>
#include <cstdint>

int fgetxattr(int file, const char* name, void* value, size_t size);
int fsetxattr(int file, const char* name, const void* value, size_t size, int flags);
int flistxattr(int file, char* namebuf, size_t size);

// Get Win32 attribute bits (FILE_ATTRIBUTE_xxx)
int fgetattr(int file, uint32_t& attr);
