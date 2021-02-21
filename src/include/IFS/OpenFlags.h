/*
 * OpenFlags.h
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
#define IFS_OPEN_FLAG_MAP(XX)                                                                                          \
	XX(Append, "Append to file")                                                                                       \
	XX(Truncate, "Create empty file")                                                                                  \
	XX(Create, "Create new file if file doesn't exist")                                                                \
	XX(Read, "Read access")                                                                                            \
	XX(Write, "Write access")

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
