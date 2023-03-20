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

namespace IFS
{
String TimeStamp::toString(const char* dtsep) const
{
	time_t t = mValue;
	struct tm* tm = localtime(&t);
	if(tm == nullptr) {
		return nullptr;
	}
	char buffer[64];
	m_snprintf(buffer, sizeof(buffer), _F("%02u/%02u/%04u%s%02u:%02u:%02u"), tm->tm_mday, tm->tm_mon + 1,
			   1900 + tm->tm_year, dtsep ?: " ", tm->tm_hour, tm->tm_min, tm->tm_sec);
	return buffer;
}

} // namespace IFS
