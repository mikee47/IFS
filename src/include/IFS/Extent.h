/****
 * Extent.h
 *
 * Copyright 2023 mikee47 <mike@sillyhouse.net>
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

#include <Storage/Types.h>

namespace IFS
{
/**
 * @brief Defines the location of a contiguous run of file data
 *
 * e.g.
 *  offset: 0
 *  length: 251
 *  skip: 5
 *  repeat: 30
 * Describes 30 blocks of data each of 251 bytes with a 5-byte gap between them.
 * Thus run contains 7530 bytes of file data, consuming 7680 bytes of storage.
 *
 * skip/repeat is necessary for SPIFFS to keep RAM usage sensible.
 * Thus, a sample 267kByte application image has 1091 extents, but requires perhaps only 60 entries.
 */
struct Extent {
	storage_size_t offset; ///< From start of partition
	uint32_t length;	   ///< In bytes
	uint16_t skip;		   ///< Skip bytes to next repeat
	uint16_t repeat;	   ///< Number of repeats
};

} // namespace IFS
