/**
 * Error.cpp
 * SPIFFS error codes
 *
 * Created on: 31 Aug 2018
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

#include <littlefs/lfs.h>
#include "../include/IFS/Types.h"
#include <FlashString/Map.hpp>

namespace IFS
{
namespace LittleFS
{
#define LFS_ERROR_MAP(XX)                                                                                              \
	XX(OK, "No error")                                                                                                 \
	XX(IO, "Error during device operation")                                                                            \
	XX(CORRUPT, "Corrupted")                                                                                           \
	XX(NOENT, "No directory entry")                                                                                    \
	XX(EXIST, "Entry already exists")                                                                                  \
	XX(NOTDIR, "Entry is not a dir")                                                                                   \
	XX(ISDIR, "Entry is a dir")                                                                                        \
	XX(NOTEMPTY, "Dir is not empty")                                                                                   \
	XX(BADF, "Bad file number")                                                                                        \
	XX(FBIG, "File too large")                                                                                         \
	XX(INVAL, "Invalid parameter")                                                                                     \
	XX(NOSPC, "No space left on device")                                                                               \
	XX(NOMEM, "No more memory available")                                                                              \
	XX(NOATTR, "No data/attr available")                                                                               \
	XX(NAMETOOLONG, "File name too long")

#define XX(tag, desc) DEFINE_FSTR_LOCAL(str_##tag, #tag)
LFS_ERROR_MAP(XX)
#undef XX

#define XX(tag, desc) {LFS_ERR_##tag, &str_##tag},
DEFINE_FSTR_MAP_LOCAL(errorMap, int, FlashString, LFS_ERROR_MAP(XX))
#undef XX

String lfsErrorToStr(int err)
{
	return errorMap[std::min(err, 0)].content();
}

} // namespace LittleFS
} // namespace IFS
