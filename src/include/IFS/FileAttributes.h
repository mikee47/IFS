/**
 * FileAttributes.h
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
 * @brief file attributes
 *
 * Archive: Unlike the dirty flag, this may be written to disk. It is typically used by applications
 * to indicate that a file has been backed up, but may have other uses. Not touched by the filesystem.
 *
 * Dirty: This is an internal attribute and indicates file content or metadata needs to be flushed to disk.
 * Applications may find it useful to determine if a file has been modified whilst open. Clearing this
 * flag before closing a file will prevent any metadata changes being flushed to disk.
 */
#define IFS_FILEATTR_MAP(XX)                                                                                           \
	XX(Compressed, C, "File content is compressed")                                                                    \
	XX(Archive, A, "File modified flag")                                                                               \
	XX(ReadOnly, R, "File may not be modified or deleted")                                                             \
	XX(Directory, D, "Object is a directory entry")                                                                    \
	XX(MountPoint, M, "Directs to another object store")                                                               \
	XX(Encrypted, E, "File is encrypted")

enum class FileAttribute {
#define XX(_tag, _char, _comment) _tag,
	IFS_FILEATTR_MAP(XX)
#undef XX
		MAX
};

/**
 * @brief File attributes are stored as a bitmask
 */
using FileAttributes = BitSet<uint8_t, FileAttribute, size_t(FileAttribute::MAX)>;

/**
 * @brief Get the string representation for the given set of file attributes
 * suitable for inclusion in a file listing
 * @param attr
 * @retval String
 */
String getFileAttributeString(FileAttributes attr);

} // namespace IFS

/**
 * @brief Get descriptive String for a given file attribute
 */
String toString(IFS::FileAttribute attr);
