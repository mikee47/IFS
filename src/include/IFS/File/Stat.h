/****
 * Stat.h
 *
 * Created August 2018 by mikee471
 *
 ****/

#pragma once

#include "../NameBuffer.h"
#include "Access.h"
#include "Compression.h"
#include "Attributes.h"

namespace IFS
{
class IFileSystem;

namespace File
{
/*
 * File handle
 *
 * References an open file
 */
using Handle = int16_t;

/*
 * File identifier
 *
 * Contained within Stat, uniquely identifies any file on the file system.
 */
using ID = uint32_t;

/*
 * File Status structure
 */
struct Stat {
	IFileSystem* fs{nullptr}; ///< The filing system owning this file
	NameBuffer name;		  ///< Name of file
	uint32_t size{0};		  ///< Size of file in bytes
	ID id{0};				  ///< Internal file identifier
	Compression compression{};
	Attributes attr{};
	ACL acl{UserRole::None, UserRole::None}; ///< Access Control
	time_t mtime{0};						 ///< File modification time

	Stat()
	{
	}

	Stat(char* namebuf, uint16_t bufsize) : name(namebuf, bufsize)
	{
	}

	/** @brief assign content from another Stat structure
	 *  @note All fields are copied as for a normal assignment, except for 'name', where
	 *  rhs.name contents are copied into our name buffer.
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

	void clear()
	{
		*this = Stat{};
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

private:
	char buffer[256];
};

} // namespace File

using FileStat = File::Stat;
using FileNameStat = File::NameStat;

} // namespace IFS
