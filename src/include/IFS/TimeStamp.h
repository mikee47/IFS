/*
 * TimeStamp.h
 */

#pragma once

#include <stdint.h>
#include <time.h>

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
		mValue = static_cast<uint32_t>(t);
		return *this;
	}
};

} // namespace IFS
