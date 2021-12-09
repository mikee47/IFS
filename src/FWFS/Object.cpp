/**
 * Object.cpp
 *
 * Created on: 26 Aug 2018
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

#include <IFS/FWFS/Object.h>
#include <FlashString/Map.hpp>

namespace IFS
{
namespace FWFS
{
constexpr uint8_t Object::FWOBT_REF;

FileAttributes getFileAttributes(Object::Attributes objattr)
{
	using ObjAttr = Object::Attribute;
	FileAttributes fileAttr{};
	fileAttr[FileAttribute::ReadOnly] = objattr[ObjAttr::ReadOnly];
	fileAttr[FileAttribute::Archive] = objattr[ObjAttr::Archive];
	fileAttr[FileAttribute::Encrypted] = objattr[ObjAttr::Encrypted];
	return fileAttr;
}

Object::Attributes getObjectAttributes(FileAttributes fileAttr)
{
	using ObjAttr = Object::Attribute;
	Object::Attributes objattr{};
	objattr[ObjAttr::ReadOnly] = fileAttr[FileAttribute::ReadOnly];
	objattr[ObjAttr::Archive] = fileAttr[FileAttribute::Archive];
	objattr[ObjAttr::Encrypted] = fileAttr[FileAttribute::Encrypted];
	return objattr;
}

} // namespace FWFS
} // namespace IFS

namespace
{
#define XX(value, tag, text) DEFINE_FSTR_LOCAL(str_##tag, #tag)
FWFS_OBJTYPE_MAP(XX)
#undef XX

using Type = IFS::FWFS::Object::Type;
#define XX(value, tag, text) {Type::tag, &str_##tag},
DEFINE_FSTR_MAP_LOCAL(typeStrings, Type, FSTR::String, FWFS_OBJTYPE_MAP(XX))
#undef XX

} // namespace

String toString(Type obt)
{
	String s = typeStrings[obt].content();
	if(!s) {
		// Custom object type?
		s = '#';
		s += unsigned(obt);
	}

	return s;
}
