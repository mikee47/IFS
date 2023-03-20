/****
 * Debug.h
 *
 * Copyright 2022 mikee47 <mike@sillyhouse.net>
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

#include "FileSystem.h"
#include <Print.h>
#include <Data/BitSet.h>

namespace IFS
{
namespace Debug
{
enum class Option {
	recurse,	// Recurse sub-directories
	attributes, // Include attributes
};

using Options = BitSet<uint8_t, Option, 2>;

void printFsInfo(Print& out, FileSystem& fs);
void printAttrInfo(Print& out, FileSystem& fs, const String& filename);
int listDirectory(Print& out, FileSystem& fs, const String& path, Options options = 0);

} // namespace Debug
} // namespace IFS
