/**
 * FileMeta.h
 *
 * Created on: 21 Jul 2018
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

#include "../Access.h"
#include "../FileAttributes.h"
#include "../Compression.h"

namespace IFS
{
namespace SPIFFS
{
/**
 * @brief Content of SPIFFS metadata area
 */
struct FileMeta {
	/// This number is made up, but serves to identify that metadata is valid
	static constexpr uint32_t Magic{0xE3457A77};

	// Magic
	uint32_t magic;
	// Modification time
	TimeStamp mtime;
	// FileAttributes - default indicates content has changed
	FileAttributes attr;
	// Security
	ACL acl;
	// Compression
	Compression compression;

	void init()
	{
		*this = FileMeta{};
		magic = Magic;
		mtime = fsGetTimeUTC();
		acl.readAccess = UserRole::Admin;
		acl.writeAccess = UserRole::Admin;
	}

	void* getAttributePtr(AttributeTag tag)
	{
		switch(tag) {
		case AttributeTag::ModifiedTime:
			return &mtime;
		case AttributeTag::Acl:
			return &acl;
		case AttributeTag::Compression:
			return &compression;
		case AttributeTag::FileAttributes:
			return &attr;
		default:
			return nullptr;
		}
	}
};

#define FILEMETA_SIZE 16
static_assert(sizeof(FileMeta) == FILEMETA_SIZE, "FileMeta wrong size");

#if SPIFFS_OBJ_META_LEN >= FILEMETA_SIZE
#define SPIFFS_STORE_META
#define SPIFFS_USER_METALEN (SPIFFS_OBJ_META_LEN - FILEMETA_SIZE)
#else
#define SPIFFS_USER_METALEN 0
#endif

struct SpiffsMetaBuffer {
	FileMeta meta;
	uint8_t user[SPIFFS_USER_METALEN];

	enum class Flag {
		dirty,
	};
	BitSet<uint8_t, Flag> flags;

	void init()
	{
		meta.init();
		memset(user, 0xFF, sizeof(user));
	}

	template <typename T> void assign(const T& data)
	{
		static_assert(sizeof(data) == offsetof(SpiffsMetaBuffer, flags), "SPIFFS metadata assign() size incorrect");
		memcpy(reinterpret_cast<void*>(this), data, sizeof(data));

		// If metadata uninitialised, then initialise it now
		if(meta.magic != FileMeta::Magic) {
			meta.init();
			flags += Flag::dirty;
		}
	}

	void copyTo(Stat& stat)
	{
		stat.acl = meta.acl;
		stat.attr = meta.attr;
		stat.mtime = meta.mtime;
		stat.compression = meta.compression;
	}

	void setFileTime(time_t t)
	{
		if(meta.mtime != t) {
			meta.mtime = t;
			flags += Flag::dirty;
		}
	}

	int getxattr(AttributeTag tag, void* buffer, size_t size)
	{
		auto attrSize = getAttributeSize(tag);
		if(attrSize == 0) {
			return Error::BadParam;
		}
		if(size >= attrSize) {
			auto value = meta.getAttributePtr(tag);
			if(value != nullptr) {
				memcpy(buffer, value, attrSize);
			}
		}
		return attrSize;
	}

	int setxattr(AttributeTag tag, const void* data, size_t size)
	{
		auto attrSize = getAttributeSize(tag);
		if(attrSize == 0) {
			return Error::BadParam;
		}
		if(size != attrSize) {
			return Error::BadParam;
		}
		auto value = meta.getAttributePtr(tag);
		if(value != nullptr) {
			memcpy(value, data, attrSize);
			if(tag == AttributeTag::Compression) {
				meta.attr[FileAttribute::Compressed] = (meta.compression.type != Compression::Type::None);
			}
		}
		return FS_OK;
	}
};

} // namespace SPIFFS
} // namespace IFS
