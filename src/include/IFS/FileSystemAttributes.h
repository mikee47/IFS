/*
 * FileSystemAttributes.h
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#pragma once

#include "ifstypes.h"

/** @brief Attribute flags for filing system
  */
#define FILE_SYSTEM_ATTR_MAP(XX)                                                                                       \
	XX(Mounted, "Filing system is mounted and in use")                                                                 \
	XX(ReadOnly, "Writing not permitted to this volume")                                                               \
	XX(Virtual, "Virtual filesystem, doesn't host files directly")                                                     \
	XX(Check, "Volume check recommended")

enum class FileSystemAttr {
#define XX(_tag, _comment) _tag,
	FILE_SYSTEM_ATTR_MAP(XX)
#undef XX
		MAX
};

// The set of attributes
typedef uint8_t FileSystemAttributes;

/** @brief Get the string representation for the given set of filesystem attributes
 *  @param attr
 *  @param buf
 *  @param bufSize
 *  @retval char* points to buf
 */
char* fileSystemAttrToStr(FileSystemAttributes attr, char* buf, size_t bufSize);
