/*
 * FileMeta.h
 *
 *  Created on: 21 Jul 2018
 *      Author: mikee47
 *
 */

#pragma once

#include "../File/Access.h"
#include "../File/Attributes.h"
#include "../File/Compression.h"

namespace IFS
{
namespace SPIFFS
{
#pragma pack(1)

/// This number is made up, but serves to identify that metadata is valid
static constexpr uint32_t metaMagic{0xE3457A77};

/** @brief Content of SPIFFS metadata area
 */
struct FileMeta {
	// Magic
	uint32_t magic;
	// Modification time
	time_t mtime;
	// File::Attributes - default indicates content has changed
	uint8_t attr;
	// Used internally, always 0xFF on disk
	uint8_t flags_;
	// Security
	File::ACL acl;

	// We use '0' for dirty so when it's clear disk gets a '1', flash default
	void setDirty()
	{
		bitClear(flags_, 0);
	}

	void clearDirty()
	{
		bitSet(flags_, 0);
	}

	bool isDirty()
	{
		return !bitRead(flags_, 0);
	}
};

#define SPIFFS_STORE_META (SPIFFS_OBJ_META_LEN >= 16)

union SpiffsMetaBuffer {
#if SPIFFS_STORE_META
	uint8_t buffer[SPIFFS_OBJ_META_LEN]{0};
#endif
	FileMeta meta;
};

#pragma pack()

} // namespace SPIFFS
} // namespace IFS
