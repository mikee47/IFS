/**
 * Util.cpp
 *
 * Created on: 10 Sep 2018
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

#include "include/IFS/Util.h"

namespace IFS
{
bool isRootPath(const char*& path)
{
	if(path == nullptr) {
		return true;
	}
	if(*path == '/') {
		++path;
	}
	if(*path == '\0') {
		path = nullptr;
		return true;
	}
	return false;
}

void checkStat(Stat& stat)
{
	stat.attr[FileAttribute::Compressed] = (stat.compression.type != Compression::Type::None);
	if(!stat.attr[FileAttribute::Compressed]) {
		stat.compression.originalSize = stat.size;
	}
}

} // namespace IFS
