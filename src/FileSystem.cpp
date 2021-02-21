/*
 * FileSystemAttributes.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/FileSystem.h"

namespace IFS
{
uint32_t FileSystem::getSize(FileHandle file)
{
	auto curpos = lseek(file, 0, SeekOrigin::Current);
	lseek(file, 0, SeekOrigin::End);
	int size = tell(file);
	lseek(file, curpos, SeekOrigin::Start);
	return (size > 0) ? uint32_t(size) : 0;
}

uint32_t FileSystem::getSize(const char* fileName)
{
	auto file = open(fileName, OpenFlag::Read);
	if(file < 0) {
		return 0;
	}
	// Get size
	lseek(file, 0, SeekOrigin::End);
	int size = tell(file);
	close(file);
	return (size > 0) ? uint32_t(size) : 0;
}

int FileSystem::truncate(const char* fileName, size_t newSize)
{
	auto file = open(fileName, OpenFlag::Write);
	if(file < 0) {
		return file;
	}

	int res = truncate(file, newSize);
	close(file);
	return res;
}

int FileSystem::setContent(const char* fileName, const char* content, size_t length)
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

	int size = lseek(file, 0, SeekOrigin::End);
	if(size == 0) {
		res = ""; // Valid String, file is empty
	} else if(size > 0) {
		lseek(file, 0, SeekOrigin::Start);
		res.setLength(size);
		if(read(file, res.begin(), res.length()) != size) {
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

	int size = lseek(file, 0, SeekOrigin::End);
	if(size <= 0 || bufSize <= size_t(size)) {
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

int FileSystem::readContent(FileHandle file, size_t size, ReadContentCallback callback)
{
	constexpr size_t bufSize{512};
	char buf[bufSize];
	size_t count{0};
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

int FileSystem::readContent(FileHandle file, ReadContentCallback callback)
{
	constexpr size_t bufSize{512};
	char buf[bufSize];
	size_t count{0};
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

int FileSystem::readContent(const String& filename, ReadContentCallback callback)
{
	auto file = open(filename, OpenFlag::Read);
	if(file < 0) {
		return file;
	}
	int res = readContent(file, callback);
	close(file);
	return res;
}

int FileSystem::makedirs(const char* path)
{
	auto pos = path;
	while(auto sep = strchr(pos, '/')) {
		String seg(pos, sep - pos);
		int err = mkdir(seg.c_str());
		if(err < 0) {
			return err;
		}
		pos = sep + 1;
	}

	return FS_OK;
}

} // namespace IFS
