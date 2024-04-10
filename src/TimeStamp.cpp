/****
 * TimeStamp.cpp
 *
 * Copyright 2022 mikee47 <mike@sillyhouse.net>
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

#include "include/IFS/TimeStamp.h"
#include <DateTime.h>

namespace IFS
{
template <typename T = time_t> typename std::enable_if<sizeof(T) == 8, time_t>::type clamp(uint32_t timestamp)
{
	return timestamp;
}

template <typename T = time_t> typename std::enable_if<sizeof(T) == 4, time_t>::type clamp(uint32_t timestamp)
{
	return (timestamp < INT32_MAX) ? timestamp : INT32_MAX;
}

String TimeStamp::toString(const char* dtsep) const
{
	DateTime dt(clamp(mValue));
	String s;
	s += dt.format(_F("%d/%m/%Y"));
	s += dtsep ?: "T";
	s += dt.format(_F("%H:%M:%S"));
	return s;
}

} // namespace IFS
