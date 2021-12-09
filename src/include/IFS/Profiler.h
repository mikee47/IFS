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

namespace IFS
{
/**
 * @brief Filesystems may optionally provide performance statistics
 */
class IProfiler
{
public:
	virtual ~IProfiler()
	{
	}

	/**
	 * @brief Called AFTER reading a block of data
	 */
	virtual void read(uint32_t address, const void* buffer, size_t size) = 0;

	/**
	 * @brief Called BEFORE writing a block of data
	 */
	virtual void write(uint32_t address, const void* buffer, size_t size) = 0;

	/**
	 * @brief Called BEFORE an erase operation
	 */
	virtual void erase(uint32_t address, size_t size) = 0;
};

class Profiler : public IProfiler
{
public:
	struct Stat {
		size_t count{0};
		size_t size{0};

		void reset()
		{
			size = count = 0;
		}

		void update(size_t n)
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
	};

	Stat readStat;
	Stat writeStat;
	Stat eraseStat;

	void read(uint32_t address, const void* buffer, size_t size) override
	{
		(void)buffer;
		readStat.update(size);
	}

	void write(uint32_t address, const void* buffer, size_t size) override
	{
		(void)buffer;
		writeStat.update(size);
	}

	void erase(uint32_t address, size_t size) override
	{
		eraseStat.update(size);
	}

	void reset()
	{
		readStat.reset();
		writeStat.reset();
		eraseStat.reset();
	}

	String toString() const
	{
		String s;
		s += F("Read: ");
		s += readStat.toString();
		s += F(", Write: ");
		s += writeStat.toString();
		s += F(", Erase: ");
		s += eraseStat.toString();
		return s;
	}
};

} // namespace IFS
