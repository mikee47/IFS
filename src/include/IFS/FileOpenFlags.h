/*
 * FileOpenFlags.h
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

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
#define FILE_OPEN_FLAG_MAP(XX)                                                                                         \
	XX(Append, "Append to file")                                                                                       \
	XX(Truncate, "Create empty file")                                                                                  \
	XX(Create, "Create new file if file doesn't exist")                                                                \
	XX(Read, "Read access")                                                                                            \
	XX(Write, "Write access")

enum class FileOpenFlag {
#define XX(_tag, _comment) _tag,
	FILE_OPEN_FLAG_MAP(XX)
#undef XX
		MAX
};

// The set of flags
using FileOpenFlags = uint8_t;

// Various file flag combinations
const FileOpenFlags eFO_ReadOnly = _BV(FileOpenFlag::Read);
const FileOpenFlags eFO_WriteOnly = _BV(FileOpenFlag::Write);
const FileOpenFlags eFO_ReadWrite = eFO_ReadOnly | eFO_WriteOnly;
const FileOpenFlags eFO_CreateIfNotExist = _BV(FileOpenFlag::Create);
const FileOpenFlags eFO_Append = _BV(FileOpenFlag::Append);
const FileOpenFlags eFO_Truncate = _BV(FileOpenFlag::Truncate);
const FileOpenFlags eFO_CreateNewAlways = _BV(FileOpenFlag::Create) | _BV(FileOpenFlag::Truncate);

/* @brief Get a string representation of a set of file open flags
 * @param flags
 * @param buf
 * @param bufSize
 * @retval char* pointer to buffer
 * @note intended for debug output
 */
char* fileOpenFlagsToStr(FileOpenFlags flags, char* buf, size_t bufSize);

} // namespace IFS
