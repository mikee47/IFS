/*
 * Compression.h
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#pragma once

#include "Types.h"

namespace IFS
{
/**
 * @brief compression type
 */
#define IFS_COMPRESSION_TYPE_MAP(XX)                                                                                       \
	XX(None, "Normal file, no compression")                                                                            \
	XX(GZip, "GZIP compressed for serving via HTTP")

/**
 * @brief A compression descriptor
 */
struct Compression {
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
