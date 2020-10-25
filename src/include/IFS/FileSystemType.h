/*
 * FileSystemType.h
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#pragma once

#include "Types.h"

namespace IFS
{
/** @brief Four-character tag to identify type of filing system
 *  @note As new filing systems are incorporated into IFS, update this enumeration
 */
#define FILESYSTEM_TYPE_MAP(XX)                                                                                        \
	XX(Unknown, NULL, "Unknown")                                                                                       \
	XX(FWFS, FWFS, "Firmware File System")                                                                             \
	XX(SPIFFS, SPIF, "SPI Flash File System (SPIFFS)")                                                                 \
	XX(Hybrid, HYFS, "Hybrid File System")

enum class FileSystemType {
#define XX(_name, _tag, _desc) _name,
	FILESYSTEM_TYPE_MAP(XX)
#undef XX
		MAX
};

/** @brief get string for filesystem type
 *  @param type
 *  @param buf buffer to store tag
 *  @param bufSize space in buffer
 *  @retval char* destination buffer
 *  @note if buffer more than 4 characters then nul will be appended
 */
char* fileSystemTypeToStr(FileSystemType type, char* buf, unsigned bufSize);

} // namespace IFS
