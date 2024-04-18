/**
 * Attribute.cpp
 *
 *  Created April 2021
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

#include "include/IFS/Attribute.h"
#include <FlashString/Vector.hpp>

namespace
{
#define XX(name, size) DEFINE_FSTR_LOCAL(str_##name, #name)
IFS_ATTRIBUTE_TAG_MAP(XX)
#undef XX

#define XX(name, size) &str_##name,
DEFINE_FSTR_VECTOR_LOCAL(strings, FSTR::String, IFS_ATTRIBUTE_TAG_MAP(XX))
#undef XX

} // namespace

namespace IFS
{
size_t getAttributeSize(AttributeTag tag)
{
	switch(tag) {
#define XX(tag, size)                                                                                                  \
	case AttributeTag::tag:                                                                                            \
		return size;
		IFS_ATTRIBUTE_TAG_MAP(XX)
#undef XX
	default:
		return 0;
	}
}

} // namespace IFS

String toString(IFS::AttributeTag tag)
{
	if(tag >= IFS::AttributeTag::User) {
		char buf[32];
		uint8_t tagIndex = unsigned(tag) - unsigned(IFS::AttributeTag::User);
		m_snprintf(buf, sizeof(buf), _F("user%02x"), tagIndex);
		return buf;
	}

	return strings[unsigned(tag)];
}

bool fromString(const char* name, IFS::AttributeTag& tag)
{
	auto namelen = strlen(name);
	if(namelen == 6 && memicmp(name, _F("user"), 4) == 0) {
		auto tagIndex = (uint8_t(unhex(name[4])) << 4) | unhex(name[5]);
		tag = IFS::AttributeTag(unsigned(IFS::AttributeTag::User) + tagIndex);
		return true;
	}

	int i = strings.indexOf(name);
	if(i < 0) {
		return false;
	}

	tag = IFS::AttributeTag(i);
	return true;
}
