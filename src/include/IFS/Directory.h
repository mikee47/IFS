/****
 * Directory.h
 *
 * Created: May 2019
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

#include "FsBase.h"

namespace IFS
{
/**
  * @brief Wrapper class for enumerating a directory
 */
class Directory : public FsBase
{
public:
	using FsBase::FsBase;

	~Directory()
	{
		close();
	}

	/**
	 * @brief Open a directory and attach this stream object to it
	 * @param dirName Default is root directory
	 * @retval bool true on success, false on error
	 * @note call getLastError() to determine cause of failure
	 */
	bool open(const String& dirName = nullptr);

	/**
	 * @brief Close directory
	 */
	void close();

	/**
	 * @brief Rewind directory stream to start so it can be re-enumerated
	 * @retval bool true on success, false on error
	 * @note call getLastError() to determine cause of failure
	 */
	bool rewind();

	/**
	 * @brief Name of directory stream is attached to
	 * @retval String invalid if stream isn't open
	 */
	const String& getDirName() const
	{
		return name;
	}

	/**
	 * @brief Determine if directory exists
	 * @retval bool true if stream is attached to a directory
	 */
	bool dirExist() const
	{
		return dir != nullptr;
	}

	/**
	 * @brief Get path with leading separator /path/to/dir
	 */
	String getPath() const;

	/**
	 * @brief Get parent directory
	 * @retval String invalid if there is no parent directory
	 */
	String getParent() const;

	int index() const
	{
		return currentIndex;
	}

	uint32_t count() const
	{
		return uint32_t(maxIndex + 1);
	}

	bool isValid() const
	{
		return currentIndex >= 0;
	}

	file_size_t size() const
	{
		return totalSize;
	}

	const Stat& stat() const
	{
		return dirStat;
	}

	bool next();

private:
	String name;
	DirHandle dir{};
	NameStat dirStat;
	int currentIndex{-1};
	int maxIndex{-1};
	file_size_t totalSize{0};
};

} // namespace IFS
