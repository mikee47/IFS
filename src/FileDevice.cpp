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
bool FileDevice::read(uint32_t address, void* buffer, size_t len)
{
	int res = fileSystem.lseek(file, address, SeekOrigin::Start);
	if(size_t(res) != address) {
		return false;
	}

	res = fileSystem.read(file, buffer, len);
	return size_t(res) == len;
}

bool FileDevice::write(uint32_t address, const void* data, size_t len)
{
	int res = fileSystem.lseek(file, address, SeekOrigin::Start);
	if(size_t(res) != address) {
		return false;
	}

	res = fileSystem.write(file, data, len);
	return size_t(res) == len;
}

bool FileDevice::erase_range(uint32_t address, size_t len)
{
	constexpr size_t bufSize{512};
	uint8_t buffer[bufSize];
	memset(buffer, 0xff, sizeof(buffer));

	int res = fileSystem.lseek(file, address, SeekOrigin::Start);
	if(size_t(res) != address) {
		return false;
	}

	while(len > 0) {
		size_t toWrite = std::min(bufSize, len);
		int res = fileSystem.write(file, buffer, toWrite);
		if(size_t(res) != toWrite) {
			return false;
		}
		len -= toWrite;
	}

	return true;
}

} // namespace Storage
