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
String TimeStamp::toString(const char* dtsep) const
{
	time_t value = mValue;
	if(sizeof(time_t) == 4 && mValue > INT32_MAX) {
		value = INT32_MAX;
	}
	DateTime dt(value);
	String s;
	s += dt.format(_F("%d/%m/%Y"));
	s += dtsep ?: "T";
	s += dt.format(_F("%H:%M:%S"));
	return s;
}

} // namespace IFS
