/*
 * Compression.h
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#pragma once

#include "../Types.h"

namespace IFS
{
namespace File
{
/**
 * @brief compression type
 */
#define COMPRESSION_TYPE_MAP(XX)                                                                                       \
	XX(None, "Normal file, no compression")                                                                            \
	XX(GZip, "GZIP compressed for serving via HTTP")

enum class Compression : uint8_t {
#define XX(_tag, _comment) _tag,
	COMPRESSION_TYPE_MAP(XX)
#undef XX
		MAX ///< Actually maxmimum value + 1...
};

} // namespace File

/**
 * @brief Get the string representation for the given compression type
 * @param type
 * @param buf
 * @param bufSize
 * @retval char* points to buf
 */
char* toString(File::Compression type, char* buf, size_t bufSize);

} // namespace IFS
