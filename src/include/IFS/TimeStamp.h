/****
 * TimeStamp.h
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

#pragma once

#include <cstdint>
#include <time.h>
#include <cstring>
#include <WString.h>

namespace IFS
{
/**
 * @brief Manage IFS timestamps stored as an unsigned 32-bit value
 *
 * A signed 32-bit value containing seconds will overflow in about 136 years.
 * time_t starts at 1970.
 *
 **/
struct TimeStamp {
	uint32_t mValue;

	operator time_t() const
	{
		return mValue;
	}

	TimeStamp& operator=(time_t t)
	{
		mValue = t;
		return *this;
	}

	/**
	 * @brief Convert time to local time for display
	 */
	String toString(const char* dtsep = " ") const;

	explicit operator String() const
	{
		return toString();
	}
};

/**
 * @brief Get current timestamp in UTC
 * @retval time_t
 * @note Filing systems must store timestamps in UTC
 * Use this function; makes porting easier.
 */
time_t fsGetTimeUTC();

} // namespace IFS
