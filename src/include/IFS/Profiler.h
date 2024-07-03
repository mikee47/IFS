/****
 * IProfiler.h - Abstract interface to implement filesystem profiling
 *
 * Copyright 2021 mikee47 <mike@sillyhouse.net>
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

#include <WString.h>
#include <Print.h>

namespace IFS
{
/**
 * @brief Filesystems may optionally provide performance statistics
 */
class IProfiler
{
public:
	virtual ~IProfiler() = default;

	/**
	 * @name Called AFTER reading a block of data
	 * @{
	 */
	virtual void read(storage_size_t address, const void* buffer, size_t size) = 0;
	/** @} */

	/**
	 * @name Called BEFORE writing a block of data
	 * @{
	 */
	virtual void write(storage_size_t address, const void* buffer, size_t size) = 0;
	/** @} */

	/**
	 * @brief Called BEFORE an erase operation
	 */
	virtual void erase(storage_size_t address, size_t size) = 0;
};

class Profiler : public IProfiler
{
public:
	struct Stat {
		size_t count{0};
		storage_size_t size{0};

		void reset()
		{
			size = count = 0;
		}

		void update(storage_size_t n)
		{
			size += n;
			++count;
		}

		String toString() const
		{
			String s;
			s += F("count=");
			s += count;
			s += F(", size=");
			s += (size + 1023) / 1024;
			s += "KB";
			return s;
		}

		operator String() const
		{
			return toString();
		}
	};

	Stat readStat;
	Stat writeStat;
	Stat eraseStat;

	void read(storage_size_t, const void*, size_t size) override
	{
		readStat.update(size);
	}

	void write(storage_size_t, const void*, size_t size) override
	{
		writeStat.update(size);
	}

	void erase(storage_size_t, size_t size) override
	{
		eraseStat.update(size);
	}

	void reset()
	{
		readStat.reset();
		writeStat.reset();
		eraseStat.reset();
	}

	size_t printTo(Print& p) const
	{
		size_t n{0};
		n += p.print(_F("Read: "));
		n += p.print(readStat);
		n += p.print(_F(", Write: "));
		n += p.print(writeStat);
		n += p.print(_F(", Erase: "));
		n += p.print(eraseStat);
		return n;
	}
};

} // namespace IFS
