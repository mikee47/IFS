/**
 * Error.cpp
 *
 * Created on: 19 Aug 2018
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

#include "include/IFS/Error.h"
#include "include/IFS/Types.h"
#include <FlashString/String.hpp>
#include <FlashString/Vector.hpp>

namespace IFS
{
namespace Error
{
/*
 * Define string table for standard IFS error codes
 */
#define XX(tag, text) DEFINE_FSTR_LOCAL(FS_##tag, #tag)
IFS_ERROR_MAP(XX)
#undef XX

#define XX(tag, text) &FS_##tag,
DEFINE_FSTR_VECTOR_LOCAL(errorStrings, FlashString, IFS_ERROR_MAP(XX))
#undef XX

String toString(int err)
{
	if(err > 0) {
		err = 0;
	} else {
		err = -err;
	}

	String s = errorStrings[err];
	if(!s) {
		String s = F("FSERR #");
		s += err;
	}

	return s;
}

} // namespace Error
} // namespace IFS
