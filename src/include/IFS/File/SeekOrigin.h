/*
 * SeekOrigin.h
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
#define FILE_SEEK_ORIGIN_MAP(XX)                                                                                       \
	XX(Start, 0, "Start of file")                                                                                      \
	XX(Current, 1, "Current position in file")                                                                         \
	XX(End, 2, "End of file")

/**
 * @brief File seek origins
 * @note these values are fixed in stone so will never change. They only need to
 * be remapped if a filing system uses different values.
 */
enum class SeekOrigin {
#define XX(tag, value, comment) tag = value,
	FILE_SEEK_ORIGIN_MAP(XX)
#undef XX
};

} // namespace File
} // namespace IFS

String toString(IFS::File::SeekOrigin origin);
