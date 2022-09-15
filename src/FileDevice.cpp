/**
 * FileDevice.cpp
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

#include "include/Storage/FileDevice.h"

namespace Storage
{
#if defined(ENABLE_STORAGE_SIZE64) && !defined(ENABLE_FILE_SIZE64)
#define CHECK_RANGE()                                                                                                  \
	if(isSize64(address) || isSize64(address + len - 1)) {                                                             \
		debug_e("[IFS] %s(0x%llx, 0x%x) range error: set ENABLE_FILE_SIZE64=1", __FUNCTION__, address, len);           \
	}
#else
#define CHECK_RANGE()
#endif

bool FileDevice::read(storage_size_t address, void* buffer, size_t len)
{
	CHECK_RANGE()

	auto res = fileSystem.lseek(file, address, SeekOrigin::Start);
	if(storage_size_t(res) != address) {
		return false;
	}

	auto count = fileSystem.read(file, buffer, len);
	return size_t(count) == len;
}

bool FileDevice::write(storage_size_t address, const void* data, size_t len)
{
	CHECK_RANGE()

	auto res = fileSystem.lseek(file, address, SeekOrigin::Start);
	if(storage_size_t(res) != address) {
		return false;
	}

	auto count = fileSystem.write(file, data, len);
	return size_t(count) == len;
}

bool FileDevice::erase_range(storage_size_t address, storage_size_t len)
{
	CHECK_RANGE()

	constexpr size_t bufSize{512};
	uint8_t buffer[bufSize];
	memset(buffer, 0xff, sizeof(buffer));

	auto res = fileSystem.lseek(file, address, SeekOrigin::Start);
	if(storage_size_t(res) != address) {
		return false;
	}

	while(len > 0) {
		size_t toWrite = std::min(storage_size_t(bufSize), len);
		int res = fileSystem.write(file, buffer, toWrite);
		if(size_t(res) != toWrite) {
			return false;
		}
		len -= toWrite;
	}

	return true;
}

} // namespace Storage
