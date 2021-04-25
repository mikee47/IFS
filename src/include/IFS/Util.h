/**
 * Util.h
 * Various bits for file system implementations to use
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

#pragma once

#include "Stat.h"

namespace IFS
{
/**
 * @brief Check if path is root directory
 * @param Path to check, set to nullptr if it's the root directory
 * @retval bool true if path is root directory
 * 
 * Paths equal to "/" or "" are empty and considered equivalent to nullptr.
 * Methods or functions can use this macro to resolve these for simpler parsing.
 */
bool isRootPath(const char*& path);

#define FS_CHECK_PATH(path) isRootPath(path);

/*
 * Methods with a DirHandle parameter use this to check and cast to locally defined FileDir*
 */
#define GET_FILEDIR()                                                                                                  \
	CHECK_MOUNTED()                                                                                                    \
	if(dir == nullptr) {                                                                                               \
		return Error::InvalidHandle;                                                                                   \
	}                                                                                                                  \
	auto d = reinterpret_cast<FileDir*>(dir);

// Final check before returning completed stat structure
void checkStat(Stat& stat);

} // namespace IFS
