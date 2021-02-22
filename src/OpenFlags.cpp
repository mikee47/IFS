/**
 * OpenFlags.cpp
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

#include "include/IFS/OpenFlags.h"
#include <FlashString/Vector.hpp>

namespace
{
#define XX(tag, comment) DEFINE_FSTR_LOCAL(flagstr_##tag, #tag)
IFS_OPEN_FLAG_MAP(XX)
#undef XX

#define XX(tag, comment) &flagstr_##tag,
DEFINE_FSTR_VECTOR_LOCAL(flagStrings, FSTR::String, IFS_OPEN_FLAG_MAP(XX))
#undef XX

} // namespace

String toString(IFS::OpenFlag flag)
{
	return flagStrings[unsigned(flag)];
}
