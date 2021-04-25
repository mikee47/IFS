/**
 * Attribute.h
 *
 * Created: April 2021
 *
 * Copyright 2021 mikee47 <mike@sillyhouse.net>
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

#include <WString.h>
#include "TimeStamp.h"
#include "FileAttributes.h"
#include "Access.h"
#include "Compression.h"

namespace IFS
{
#define IFS_ATTRIBUTE_TAG_MAP(XX)                                                                                      \
	XX(ModifiedTime, sizeof(TimeStamp))                                                                                \
	XX(FileAttributes, sizeof(FileAttributes))                                                                         \
	XX(Acl, sizeof(ACL))                                                                                               \
	XX(Compression, sizeof(Compression))

/**
 * @brief Identifies a specific attribute
 */
enum class AttributeTag : uint16_t {
#define XX(tag, size) tag,
	IFS_ATTRIBUTE_TAG_MAP(XX)
#undef XX
		User = 16, ///< First user attribute
};

/**
 * @brief Attribute information passed to enumeration callback
 */
struct AttributeEnum {
	AttributeTag tag{}; ///< The attribute tag
	size_t size{0};		///< Size of returned data, may be less than attrsize if buffer too small
	size_t attrsize{0}; ///< Actual attribute size
	void* buffer;		///< User-provided buffer with tag value
	size_t bufsize;		///< User-provided buffer size

	AttributeEnum(void* buffer, size_t bufsize) : buffer(buffer), bufsize(bufsize)
	{
	}

	void set(AttributeTag tag, const void* value, size_t valueSize)
	{
		this->tag = tag;
		attrsize = valueSize;
		size = std::min(attrsize, bufsize);
		memcpy(buffer, value, size);
	}

	template <typename T> void set(AttributeTag tag, const T& value)
	{
		set(tag, &value, sizeof(value));
	}
};

/**
 * @brief Return true to continue enumeration, false to stop
 */
using AttributeEnumCallback = Delegate<bool(AttributeEnum& e)>;

inline AttributeTag getUserAttributeTag(uint8_t value)
{
	unsigned tagValue = value + unsigned(AttributeTag::User);
	return AttributeTag(tagValue);
}

size_t getAttributeSize(AttributeTag tag);

} // namespace IFS

String toString(IFS::AttributeTag tag);
bool fromString(const char* name, IFS::AttributeTag& tag);
