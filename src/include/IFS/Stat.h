/****
 * Stat.h
 *
 * Created: August 2018
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

#include "NameBuffer.h"
#include "TimeStamp.h"
#include "Access.h"
#include "Compression.h"
#include "FileAttributes.h"

namespace IFS
{
class IFileSystem;

/**
 * @brief File handle
 *
 * References an open file
 */
using FileHandle = int16_t;

/**
 * @brief File identifier
 *
 * Contained within Stat, uniquely identifies any file on the file system.
 */
using FileID = uint32_t;

/**
 * @brief File Status structure
 */
struct Stat {
	IFileSystem* fs{nullptr};				 ///< The filing system owning this file
	NameBuffer name;						 ///< Name of file
	uint32_t size{0};						 ///< Size of file in bytes
	FileID id{0};							 ///< Internal file identifier
	TimeStamp mtime{};						 ///< File modification time
	ACL acl{UserRole::None, UserRole::None}; ///< Access Control
	FileAttributes attr{};
	Compression compression{};

	Stat()
	{
	}

	Stat(char* namebuf, uint16_t bufsize) : name(namebuf, bufsize)
	{
	}

	/**
	 * @brief assign content from another Stat structure
	 * @note All fields are copied as for a normal assignment, except for 'name', where
	 * rhs.name contents are copied into our name buffer.
	 */
	Stat& operator=(const Stat& rhs)
	{
		fs = rhs.fs;
		name.copy(rhs.name);
		size = rhs.size;
		id = rhs.id;
		compression = rhs.compression;
		attr = rhs.attr;
		acl = rhs.acl;
		mtime = rhs.mtime;
		return *this;
	}

	bool isDir() const
	{
		return attr[FileAttribute::Directory];
	}
};

/**
 * @brief version of Stat with integrated name buffer
 * @note provide for convenience
 */
struct NameStat : public Stat {
public:
	NameStat() : Stat(buffer, sizeof(buffer))
	{
	}

	NameStat(const Stat& other) : NameStat()
	{
		*this = other;
	}

	NameStat& operator=(const Stat& rhs)
	{
		*static_cast<Stat*>(this) = rhs;
		return *this;
	}

private:
	char buffer[256];
};

} // namespace IFS
