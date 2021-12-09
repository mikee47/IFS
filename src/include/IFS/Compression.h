/****
 * Compression.h
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
/**
 * @brief compression type
 */
#define IFS_COMPRESSION_TYPE_MAP(XX)                                                                                   \
	XX(None, "Normal file, no compression")                                                                            \
	XX(GZip, "GZIP compressed for serving via HTTP")

/**
 * @brief A compression descriptor
 */
struct __attribute__((packed)) Compression {
	enum class Type : uint8_t {
#define XX(_tag, _comment) _tag,
		IFS_COMPRESSION_TYPE_MAP(XX)
#undef XX
			MAX ///< Actually maxmimum value + 1...
	};
	Type type;
	uint32_t originalSize;

	bool operator==(const Compression& other) const
	{
		return type == other.type && originalSize == other.originalSize;
	}

	bool operator!=(const Compression& other) const
	{
		return !operator==(other);
	}
};

static_assert(sizeof(Compression) == 5, "Compression wrong size");

/**
 * @name Return compression corresponding to given string
 * @param str
 * @retval Compression::Type
 * @{
 */
Compression::Type getCompressionType(const char* str, Compression::Type defaultValue = Compression::Type::None);

inline Compression::Type getCompressionType(const String& str, Compression::Type defaultValue = Compression::Type::None)
{
	return getCompressionType(str.c_str(), defaultValue);
}

/** @} */

} // namespace IFS

/**
 * @brief Get the string representation for the given compression type
 * @retval String
 */
String toString(IFS::Compression::Type type);
