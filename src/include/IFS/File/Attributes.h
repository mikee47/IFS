/*
 * Attributes.h
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
 * @brief file attributes
 *
 * Archive: Unlike the dirty flag, this may be written to disk. It is typically used by applications
 * to indicate that a file has been backed up, but may have other uses. Not touched by the filesystem.
 *
 * Dirty: This is an internal attribute and indicates file content or metadata needs to be flushed to disk.
 * Applications may find it useful to determine if a file has been modified whilst open. Clearing this
 * flag before closing a file will prevent any metadata changes being flushed to disk.
 */
#define FILEATTR_MAP(XX)                                                                                               \
	XX(Compressed, C, "File content is compressed")                                                                    \
	XX(Archive, A, "File modified flag")                                                                               \
	XX(ReadOnly, R, "File may not be modified or deleted")                                                             \
	XX(Directory, D, "Object is a directory entry")                                                                    \
	XX(MountPoint, M, "Directs to another object store")

enum class Attribute {
#define XX(_tag, _char, _comment) _tag,
	FILEATTR_MAP(XX)
#undef XX
};

/**
 * @brief File attributes are stored as a bitmask
 */
using Attributes = BitSet<uint8_t, Attribute>;

} // namespace File

/**
 * @brief Get the string representation for the given set of file attributes
 * suitable for inclusion in a file listing
 * @param attr
 * @retval String
 */
String toString(File::Attributes attr);

} // namespace IFS
