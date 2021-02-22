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

#include "include/IFS/Object.h"
#include <FlashString/Vector.hpp>

namespace IFS
{
constexpr uint8_t Object::FWOBT_REF;
}

namespace
{
#define XX(value, tag, text) DEFINE_FSTR_LOCAL(str_##tag, #tag)
FWFS_OBJTYPE_MAP(XX)
#undef XX

#define XX(value, tag, text) &str_##tag,
DEFINE_FSTR_VECTOR_LOCAL(typeStrings, FSTR::String, FWFS_OBJTYPE_MAP(XX))
#undef XX

} // namespace

String toString(IFS::Object::Type obt)
{
	String s = typeStrings[unsigned(obt)];
	if(!s) {
		// Custom object type?
		s = '#';
		s += unsigned(obt);
	}

	return s;
}
