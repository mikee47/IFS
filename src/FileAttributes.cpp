/**
 * Attributes.cpp
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

#include "include/IFS/FileAttributes.h"
#include <FlashString/Vector.hpp>

namespace
{
#define XX(tag, ch, comment) #ch
DEFINE_FSTR_LOCAL(attributeChars, IFS_FILEATTR_MAP(XX))
#undef XX

#define XX(tag, ch, comment) DEFINE_FSTR_LOCAL(attstr_##tag, #tag)
IFS_FILEATTR_MAP(XX)
#undef XX

#define XX(tag, ch, comment) &attstr_##tag,
DEFINE_FSTR_VECTOR_LOCAL(attributeStrings, FSTR::String, IFS_FILEATTR_MAP(XX))
#undef XX

} // namespace

namespace IFS
{
String getFileAttributeString(FileAttributes attr)
{
	String s = attributeChars;

	for(unsigned a = 0; a < s.length(); ++a) {
		if(!attr[FileAttribute(a)]) {
			s[a] = '.';
		}
	}

	return s;
}

} // namespace IFS

String toString(IFS::FileAttribute attr)
{
	String s = attributeStrings[unsigned(attr)];
	return s ?: F("UNK#") + String(unsigned(attr));
}
