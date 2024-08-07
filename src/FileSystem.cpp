/**
 * FileSystemAttributes.cpp
 *
 * Created on: 31 Aug 2018
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

#include "include/IFS/FileSystem.h"

namespace IFS
{
file_size_t FileSystem::getSize(FileHandle file)
{
	auto curpos = lseek(file, 0, SeekOrigin::Current);
	auto size = lseek(file, 0, SeekOrigin::End);
	lseek(file, curpos, SeekOrigin::Start);
	return (size > 0) ? size : 0;
}

file_size_t FileSystem::getSize(const char* fileName)
{
	auto file = open(fileName, OpenFlag::Read);
	if(file < 0) {
		return 0;
	}
	// Get size
	auto size = lseek(file, 0, SeekOrigin::End);
	close(file);
	return (size > 0) ? size : 0;
}

int FileSystem::truncate(const char* fileName, file_size_t newSize)
{
	auto file = open(fileName, OpenFlag::Write);
	if(file < 0) {
		return file;
	}

	int res = ftruncate(file, newSize);
	close(file);
	return res;
}

int FileSystem::setContent(const char* fileName, const void* content, size_t length)
{
	auto file = open(fileName, OpenFlag::Create | OpenFlag::Truncate | OpenFlag::Write);
	if(file < 0) {
		return file;
	}

	int res = write(file, content, length);
	close(file);
	return res;
}

String FileSystem::getContent(const String& fileName)
{
	String res;
	auto file = open(fileName, OpenFlag::Read);
	if(file < 0) {
		return nullptr;
	}

	auto size = lseek(file, 0, SeekOrigin::End);
	if(size == 0) {
		res = ""; // Valid String, file is empty
	} else if(size > 0) {
		lseek(file, 0, SeekOrigin::Start);
		if(size < 0x100000 && res.setLength(size) && read(file, res.begin(), res.length()) != size) {
			res = nullptr; // read failed, invalidate String
		}
	}
	close(file);
	return res;
}

size_t FileSystem::getContent(const char* fileName, char* buffer, size_t bufSize)
{
	if(buffer == nullptr || bufSize == 0) {
		return 0;
	}

	auto file = open(fileName, OpenFlag::Read);
	if(file < 0) {
		buffer[0] = '\0';
		return 0;
	}

	auto size = lseek(file, 0, SeekOrigin::End);
	if(size <= 0 || file_size_t(size) > bufSize) {
		size = 0;
	} else {
		lseek(file, 0, SeekOrigin::Start);
		if(read(file, buffer, size) != size) {
			size = 0; // Error
		}
	}
	close(file);
	buffer[size] = '\0';
	return size;
}

file_offset_t FileSystem::readContent(FileHandle file, size_t size, const ReadContentCallback& callback)
{
	constexpr size_t bufSize{512};
	char buf[bufSize];
	file_offset_t count{0};
	while(size > 0) {
		size_t toRead = std::min(bufSize, size);
		int len = read(file, buf, toRead);
		if(len < 0) {
			return len;
		}
		if(len == 0) {
			break;
		}
		int res = callback(buf, len);
		if(res < 0) {
			return res;
		}
		count += size_t(len);
		size -= size_t(len);
	}

	return count;
}

file_offset_t FileSystem::readContent(FileHandle file, const ReadContentCallback& callback)
{
	constexpr size_t bufSize{512};
	char buf[bufSize];
	file_offset_t count{0};
	int len;
	while((len = read(file, buf, bufSize)) > 0) {
		int res = callback(buf, len);
		if(res < 0) {
			return res;
		}
		count += size_t(len);
	}

	return (len < 0) ? len : count;
}

file_offset_t FileSystem::readContent(const String& filename, const ReadContentCallback& callback)
{
	auto file = open(filename, OpenFlag::Read);
	if(file < 0) {
		return file;
	}
	auto res = readContent(file, callback);
	close(file);
	return res;
}

int FileSystem::makedirs(const char* path)
{
	String s(path);
	int i = 0;
	while((i = s.indexOf('/', i)) > 0) {
		s[i] = '\0';
		int err = mkdir(s.c_str());
		if(err < 0) {
			return err;
		}
		s[i++] = '/';
	}

	return FS_OK;
}

} // namespace IFS
