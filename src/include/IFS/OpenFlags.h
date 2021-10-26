/****
 * OpenFlags.h
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

#pragma once

#include "Types.h"

namespace IFS
{
/** @brief File open flag
 *  @note These are filing-system independent flags based on SPIFFS 0.3.7, however they
 *  may change so filing systems should map them.
 *  A filing system must fail the call if any flags are not recognised.
 *  Flags are defined as bit values.
 */
#define IFS_OPEN_FLAG_MAP(XX)                                                                                          \
	XX(Append, "Append to file")                                                                                       \
	XX(Truncate, "Create empty file")                                                                                  \
	XX(Create, "Create new file if file doesn't exist")                                                                \
	XX(Read, "Read access")                                                                                            \
	XX(Write, "Write access")                                                                                          \
	XX(NoFollow, "Don't follow symbolic links")

enum class OpenFlag {
#define XX(_tag, _comment) _tag,
	IFS_OPEN_FLAG_MAP(XX)
#undef XX
		MAX
};

// The set of flags
using OpenFlags = BitSet<uint8_t, OpenFlag, size_t(OpenFlag::MAX)>;

inline constexpr OpenFlags operator|(OpenFlag a, OpenFlag b)
{
	return OpenFlags(a) | b;
}

} // namespace IFS

/**
 * @brief Get a descriptive string for a flag
 */
String toString(IFS::OpenFlag flag);
