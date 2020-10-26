/*
 * FileFlags.h
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#pragma once

#include "Types.h"
#include "Data/BitSet.h"

namespace IFS
{
/** @brief File seek origin flags
 *  @note these values are fixed in stone so will never change. They only need to
 *  be remapped if a filing system uses different values.
 */
enum class SeekOrigin {
	Start = 0,   ///< Start of file
	Current = 1, ///< Current position in file
	End = 2		 ///< End of file
};

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
using FileOpenFlags = BitSet<uint8_t, IFS::FileOpenFlag>;

inline constexpr FileOpenFlags operator|(FileOpenFlag a, FileOpenFlag b)
{
	return FileOpenFlags{a} | FileOpenFlags{b};
}

/**
 * @brief Get a string representation of a set of file open flags
 * @param flags
 * @param buf
 * @param bufSize
 * @retval char* pointer to buffer
 * @note intended for debug output
 */
char* toString(FileOpenFlags flags, char* buf, size_t bufSize);

} // namespace IFS
