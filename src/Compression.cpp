/**
 * Compression.cpp
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

#include "include/IFS/Compression.h"
#include <FlashString/String.hpp>
#include <FlashString/Vector.hpp>

namespace
{
#define XX(tag, comment) DEFINE_FSTR_LOCAL(str_##tag, #tag)
IFS_COMPRESSION_TYPE_MAP(XX)
#undef XX

#define XX(tag, comment) &str_##tag,
DEFINE_FSTR_VECTOR_LOCAL(strings, FSTR::String, IFS_COMPRESSION_TYPE_MAP(XX))
#undef XX

} // namespace

namespace IFS
{
Compression::Type getCompressionType(const char* str, Compression::Type defaultValue)
{
	int i = strings.indexOf(str);
	return (i < 0) ? defaultValue : Compression::Type(i);
}

} // namespace IFS

String toString(IFS::Compression::Type type)
{
	String s = strings[unsigned(type)];
	return s ?: F("UNK#") + String(unsigned(type));
}
