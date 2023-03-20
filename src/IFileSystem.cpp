/**
 * IFileSystem.cpp
 *
 *  Created on: 31 Aug 2018
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

#include "include/IFS/IFileSystem.h"
#include <FlashString/Vector.hpp>

namespace
{
#define XX(name, tag, desc) DEFINE_FSTR_LOCAL(typestr_##name, #tag)
FILESYSTEM_TYPE_MAP(XX)
#undef XX

#define XX(name, tag, desc) &typestr_##name,
DEFINE_FSTR_VECTOR_LOCAL(typeStrings, FSTR::String, FILESYSTEM_TYPE_MAP(XX))
#undef XX

#define XX(tag, comment) DEFINE_FSTR_LOCAL(attrstr_##tag, #tag)
FILE_SYSTEM_ATTR_MAP(XX)
#undef XX

#define XX(tag, comment) &attrstr_##tag,
DEFINE_FSTR_VECTOR_LOCAL(attributeStrings, FSTR::String, FILE_SYSTEM_ATTR_MAP(XX))
#undef XX

template <typename T, typename... Args> size_t tprintln(Print& p, String tag, const T& value, Args... args)
{
	size_t n{0};
	n += p.print(tag.padRight(16));
	n += p.print(": ");
	n += p.println(value, args...);
	return n;
}

} // namespace

String toString(IFS::IFileSystem::Type type)
{
	if(type >= IFS::IFileSystem::Type::MAX) {
		type = IFS::IFileSystem::Type::Unknown;
	}
	return typeStrings[(unsigned)type];
}

String toString(IFS::IFileSystem::Attribute attr)
{
	return attributeStrings[unsigned(attr)];
}

namespace IFS
{
size_t IFileSystem::Info::printTo(Print& p) const
{
	size_t n{0};

#define TPRINTLN(tag, value, ...) n += tprintln(p, F(tag), value, ##__VA_ARGS__)

	TPRINTLN("type", type);
	if(partition) {
		TPRINTLN("partition", partition);
	}
	TPRINTLN("maxNameLength", maxNameLength);
	TPRINTLN("maxPathLength", maxPathLength);
	TPRINTLN("attr", toString(attr));
	TPRINTLN("volumeID", volumeID, HEX, 8);
	TPRINTLN("name", name);
	TPRINTLN("volumeSize", volumeSize);
	TPRINTLN("freeSpace", freeSpace);

	return n;
}

} // namespace IFS
