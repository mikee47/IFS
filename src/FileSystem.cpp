/*
 * FileSystemAttributes.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/FileSystem.h"
#include <FlashString/Vector.hpp>

namespace
{
#define XX(name, tag, desc) DEFINE_FSTR_LOCAL(typestr_##name, #tag)
FILESYSTEM_TYPE_MAP(XX)
#undef XX

#define XX(name, tag, desc) &typestr_##name,
DEFINE_FSTR_VECTOR_LOCAL(typeStrings, FSTR::String, FILESYSTEM_TYPE_MAP(XX))
#undef XX

#define XX(tag, comment) DEFINE_FSTR_LOCAL(attrstr_##tag, #tag)
FILE_SYSTEM_ATTR_MAP(XX)
#undef XX

#define XX(tag, comment) &attrstr_##tag,
DEFINE_FSTR_VECTOR_LOCAL(attributeStrings, FSTR::String, FILE_SYSTEM_ATTR_MAP(XX))
#undef XX
} // namespace

namespace IFS
{
uint32_t IFileSystem::getSize(File::Handle file)
{
	auto curpos = lseek(file, 0, SeekOrigin::Current);
	lseek(file, 0, SeekOrigin::End);
	int size = tell(file);
	lseek(file, curpos, SeekOrigin::Start);
	return (size > 0) ? uint32_t(size) : 0;
}

uint32_t IFileSystem::getSize(const char* fileName)
{
	auto file = open(fileName, File::OpenFlag::Read);
	if(file < 0) {
		return 0;
	}
	// Get size
	lseek(file, 0, SeekOrigin::End);
	int size = tell(file);
	close(file);
	return (size > 0) ? uint32_t(size) : 0;
}

int IFileSystem::truncate(const char* fileName, size_t newSize)
{
	auto file = open(fileName, File::OpenFlag::Write);
	if(file < 0) {
		return file;
	}

	int res = truncate(file, newSize);
	close(file);
	return res;
}

int IFileSystem::setContent(const char* fileName, const char* content, size_t length)
{
	auto file = open(fileName, File::OpenFlag::Create | File::OpenFlag::Truncate | File::OpenFlag::Write);
	if(file < 0) {
		return file;
	}

	int res = write(file, content, length);
	close(file);
	return res;
}

String IFileSystem::getContent(const String& fileName)
{
	String res;
	auto file = open(fileName, File::OpenFlag::Read);
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

size_t IFileSystem::getContent(const char* fileName, char* buffer, size_t bufSize)
{
	if(buffer == nullptr || bufSize == 0) {
		return 0;
	}

	auto file = open(fileName, File::OpenFlag::Read);
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

int IFileSystem::makedirs(const char* path)
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

String toString(IFS::IFileSystem::Type type)
{
	if(type >= IFS::IFileSystem::Type::MAX) {
		type = IFS::IFileSystem::Type::Unknown;
	}
	return typeStrings[(unsigned)type];
}

String toString(IFS::IFileSystem::Attribute attr)
{
	return attributeStrings[unsigned(attr)];
}
