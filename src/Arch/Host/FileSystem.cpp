/**
 * FileSystem.cpp
 *
 * Created on: 11 Sep 2018
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

#define _POSIX_C_SOURCE 200112L

#include <IFS/Host/FileSystem.h>
#include <IFS/Host/Util.h>
#include <IFS/Util.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef __WIN32
#include <sys/utime.h>
#include "Windows/xattr.h"
#include "Windows/fsync.h"
#else
#include <sys/xattr.h>
#include <utime.h>
#endif

// We need name and path limits which work on all Host OS/s
#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

namespace IFS
{
class FileSystem;

namespace Host
{
struct FileDir {
	CString path;
	DIR* d;
};

namespace
{
// Always mounted
#define CHECK_MOUNTED()

FileSystem hostFileSystem;

const char* extendedAttributeName{"user.sming.ifs"};

/**
 * @brief Stored with file
 */
struct ExtendedAttributes {
	uint32_t size; ///< Size of this structure
	ACL acl;
	FileAttributes attr;
	IFS::Compression compression;

	void* getAttributePtr(AttributeTag tag)
	{
		switch(tag) {
		case AttributeTag::Acl:
			return &acl;
		case AttributeTag::Compression:
			return &compression;
		case AttributeTag::FileAttributes:
			return &attr;
		default:
			return nullptr;
		}
	}
};

int setXAttr(FileHandle file, const char* name, const void* value, size_t size)
{
#if defined(__MACOS)
	int res = ::fsetxattr(file, name, value, size, 0, 0);
#else
	int res = ::fsetxattr(file, name, value, size, 0);
#endif
	return (res == 0) ? FS_OK : syserr();
}

int getXAttr(FileHandle file, const char* name, void* value, size_t size)
{
#if defined(__MACOS)
	auto len = ::fgetxattr(file, name, value, size, 0, 0);
#else
	auto len = ::fgetxattr(file, name, value, size);
#endif
	return (len >= 0) ? len : syserr();
}

int listXAttr(FileHandle file, char* namebuf, size_t size)
{
#if defined(__MACOS)
	auto len = ::flistxattr(file, namebuf, size, 0);
#else
	auto len = ::flistxattr(file, namebuf, size);
#endif
	return (len >= 0) ? len : syserr();
}

int getExtendedAttributes(FileHandle file, ExtendedAttributes& ea)
{
#ifdef __WIN32
	uint32_t attr{0};
	if(fgetattr(file, attr) == 0) {
		ea.attr[FileAttribute::ReadOnly] = (attr & _A_RDONLY);
		ea.attr[FileAttribute::Archive] = (attr & _A_ARCH);
	}
#endif
	auto len = getXAttr(file, extendedAttributeName, &ea, sizeof(ea));
	if(len < 0) {
		return len;
	}
	if(len != sizeof(ea) || ea.size != sizeof(ea)) {
		return Error::ReadFailure;
	}
	return FS_OK;
}

int getExtendedAttributes(FileHandle file, Stat& stat)
{
	ExtendedAttributes ea{};
	int res = getExtendedAttributes(file, ea);
	if(res < 0) {
		return res;
	}

	stat.acl = ea.acl;
	stat.compression = ea.compression;
	stat.attr = ea.attr;
	checkStat(stat);

	if(!stat.attr[FileAttribute::Compressed]) {
		stat.compression.originalSize = stat.size;
	}

	return FS_OK;
}

int getExtendedAttributes(const char* path, Stat& stat)
{
	auto f = hostFileSystem.open(path, OpenFlag::Read);
	if(f < 0) {
		return f;
	}

	int res = getExtendedAttributes(f, stat);

	hostFileSystem.close(f);
	return res;
}

int settime(FileHandle file, TimeStamp mtime)
{
#ifdef __WIN32
	_utimbuf times{mtime, mtime};
	int res = _futime(file, &times);
#else
	struct timespec times[]{mtime, mtime};
	int res = ::futimens(file, times);
#endif
	return (res >= 0) ? res : syserr();
}

int settime(const char* path, TimeStamp mtime)
{
#ifdef __WIN32
	_utimbuf times{mtime, mtime};
	int res = _utime(path, &times);
#else
	struct utimbuf times[]{mtime, mtime};
	int res = ::utime(path, times);
#endif
	return (res >= 0) ? res : syserr();
}

} // namespace

IFileSystem& fileSystem{hostFileSystem};

IFS::FileSystem& getFileSystem()
{
	return reinterpret_cast<IFS::FileSystem&>(hostFileSystem);
}

int FileSystem::getinfo(Info& info)
{
	info.clear();
	info.type = Type::Host;
	info.maxNameLength = NAME_MAX;
	info.maxPathLength = PATH_MAX;
	info.attr = Attribute::Mounted;
	return FS_OK;
}

String FileSystem::getErrorString(int err)
{
	return IFS::Host::getErrorString(err);
}

int FileSystem::opendir(const char* path, DirHandle& dir)
{
	auto d = new FileDir{};

	// Interpret empty path as current directory
	if(path == nullptr || path[0] == '\0') {
		path = ".";
	}

	d->d = ::opendir(path);
	if(d->d == nullptr) {
		int err = syserr();
		delete d;
		return err;
	}

	d->path = path;
	dir = DirHandle(d);
	return FS_OK;
}

int FileSystem::rewinddir(DirHandle dir)
{
	GET_FILEDIR()
	::rewinddir(d->d);
	return FS_OK;
}

int FileSystem::readdir(DirHandle dir, Stat& stat)
{
	GET_FILEDIR()

	while(true) {
		errno = 0;
		dirent* e = ::readdir(d->d);
		if(e == nullptr) {
			return syserr() ?: Error::NoMoreFiles;
		}

		if(strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
			continue;
		}

		String path = d->path.c_str();
		path += '/';
		path += e->d_name;
		int res = this->stat(path.c_str(), &stat);
		return res;
	}
}

int FileSystem::closedir(DirHandle dir)
{
	GET_FILEDIR()

	int res = ::closedir(d->d);
	if(res < 0) {
		res = syserr();
	}
	delete d;
	return res;
}

int FileSystem::mkdir(const char* path)
{
#ifdef __WIN32
	int res = ::mkdir(path);
#else
	int res = ::mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
	return (res >= 0) ? res : syserr();
}

void FileSystem::fillStat(const struct stat& s, Stat& stat)
{
	stat = Stat{};
	stat.fs = this;
	stat.id = s.st_ino;
	if((s.st_mode & S_IWUSR) == 0) {
		stat.attr |= FileAttribute::ReadOnly;
	}
	if(S_ISDIR(s.st_mode)) {
		stat.attr |= FileAttribute::Directory;
	}
	stat.mtime = s.st_mtime;
	stat.size = s.st_size;
}

int FileSystem::stat(const char* path, Stat* stat)
{
	struct stat s;
	int res = ::stat(path, &s);
	if(res < 0) {
		return syserr();
	}

	if(stat != nullptr) {
		fillStat(s, *stat);
		const char* lastSep = strrchr(path, '/');
		stat->name.copy(lastSep ? lastSep + 1 : path);
		getExtendedAttributes(path, *stat);
	}

	return FS_OK;
}

int FileSystem::fstat(FileHandle file, Stat* stat)
{
	struct stat s;
	int res = ::fstat(file, &s);
	if(res < 0) {
		return syserr();
	}

	if(stat != nullptr) {
		fillStat(s, *stat);
		getExtendedAttributes(file, *stat);
	}

	return res;
}

int FileSystem::fsetxattr(FileHandle file, AttributeTag tag, const void* data, size_t size)
{
	if(tag >= AttributeTag::User) {
		char name[16];
		m_snprintf(name, sizeof(name), "__user_%02x", tag);
		return setXAttr(file, name, data, size);
	}

	auto attrSize = getAttributeSize(tag);
	if(attrSize == 0) {
		return Error::BadParam;
	}
	if(size != attrSize) {
		return Error::BadParam;
	}
	if(tag == AttributeTag::ModifiedTime) {
		TimeStamp mtime;
		memcpy(&mtime, data, size);
		return settime(file, mtime);
	}

	ExtendedAttributes ea{};
	auto value = ea.getAttributePtr(tag);
	if(value == nullptr) {
		return Error::BadParam;
	}
	int err = getExtendedAttributes(file, ea);
	if(err < 0) {
		debug_w("getExtendedAttributes: %d", err);
		return err;
	}
	memcpy(value, data, attrSize);
	ea.size = sizeof(ea);
	return setXAttr(file, extendedAttributeName, &ea, sizeof(ea));
}

int FileSystem::fgetxattr(FileHandle file, AttributeTag tag, void* buffer, size_t size)
{
	if(tag >= AttributeTag::User) {
		char name[16];
		m_snprintf(name, sizeof(name), "__user_%02x", tag);
		return getXAttr(file, name, buffer, size);
	}

	ExtendedAttributes ea{};
	void* value;
	TimeStamp mtime;
	if(tag == AttributeTag::ModifiedTime) {
		struct ::stat s {
		};
		::fstat(file, &s);
		mtime = s.st_mtime;
		value = &mtime;
	} else {
		value = ea.getAttributePtr(tag);
		if(value == nullptr) {
			return Error::BadParam;
		}
		int res = getExtendedAttributes(file, ea);
		if(res < 0) {
			return res;
		}
	}
	auto attrsize = getAttributeSize(tag);
	memcpy(buffer, value, std::min(size, attrsize));
	return attrsize;
}

int FileSystem::fenumxattr(FileHandle file, AttributeEnumCallback callback, void* buffer, size_t bufsize)
{
	unsigned count{0};
	AttributeEnum e{buffer, bufsize};

	TimeStamp mtime;
	struct ::stat s {
	};
	::fstat(file, &s);
	mtime = s.st_mtime;
	for(unsigned i = 0; i < unsigned(AttributeTag::User); ++i) {
		auto tag = AttributeTag(i);
		ExtendedAttributes ea{};
		void* value;
		if(tag == AttributeTag::ModifiedTime) {
			value = &mtime;
		} else {
			value = ea.getAttributePtr(tag);
			if(value == nullptr) {
				continue;
			}
		}
		++count;
		e.set(tag, value, getAttributeSize(tag));
		if(!callback(e)) {
			return count;
		}
	}

	char names[1024];
	int listlen = listXAttr(file, names, sizeof(names));
	if(listlen < 0) {
		return listlen;
	}
	for(unsigned offset = 0; offset < unsigned(listlen);) {
		auto name = &names[offset];
		auto namelen = strlen(name);
		offset += namelen + 1;
		// Convert name to tag
		if(namelen != 9 || memcmp(name, "__user_", 7) != 0) {
			continue;
		}
		auto tagIndex = (unhex(name[7]) << 4) | unhex(name[8]);
		e.tag = AttributeTag(unsigned(AttributeTag::User) + tagIndex);
		int attrsize = getXAttr(file, name, e.buffer, e.bufsize);
		if(attrsize < 0) {
			// TODO: Report to user... zero size?
			continue;
		}
		e.attrsize = attrsize;
		e.size = std::min(size_t(attrsize), bufsize);
		if(!callback(e)) {
			return count;
		}
	}

	return count;
}

int FileSystem::setxattr(const char* path, AttributeTag tag, const void* data, size_t size)
{
	if(tag == AttributeTag::ModifiedTime) {
		TimeStamp mtime;
		if(size < sizeof(mtime)) {
			return Error::BadParam;
		}
		memcpy(&mtime, data, size);
		return settime(path, mtime);
	}

	int file = ::open(path, O_RDWR);
	if(file < 0) {
		return syserr();
	}

	int res = fsetxattr(file, tag, data, size);

	::close(file);

	return res;
}

int FileSystem::getxattr(const char* path, AttributeTag tag, void* buffer, size_t size)
{
	int file = ::open(path, O_RDONLY);
	if(file < 0) {
		return syserr();
	}

	int res = fgetxattr(file, tag, buffer, size);

	::close(file);

	return res;
}

FileHandle FileSystem::open(const char* path, OpenFlags flags)
{
	int res = ::open(path, mapFlags(flags), 0644);
	assert(FileHandle(res) == res);
	return (res >= 0) ? res : syserr();
}

int FileSystem::close(FileHandle file)
{
	int res = ::close(file);
	return (res >= 0) ? res : syserr();
}

int FileSystem::read(FileHandle file, void* data, size_t size)
{
	int res = ::read(file, data, size);
	return (res >= 0) ? res : syserr();
}

int FileSystem::write(FileHandle file, const void* data, size_t size)
{
	int res = ::write(file, data, size);
	return (res >= 0) ? res : syserr();
}

int FileSystem::lseek(FileHandle file, int offset, SeekOrigin origin)
{
	int res = ::lseek(file, offset, uint8_t(origin));
	return (res >= 0) ? res : syserr();
}

int FileSystem::eof(FileHandle file)
{
	// POSIX doesn't appear to have eof()

	int pos = tell(file);
	if(pos < 0) {
		return syserr();
	}

	struct stat stat;
	int err = ::fstat(file, &stat);
	if(err < 0) {
		return syserr();
	}

	return (pos >= stat.st_size) ? 1 : 0;
}

int32_t FileSystem::tell(FileHandle file)
{
	return lseek(file, 0, SeekOrigin::Current);
}

int FileSystem::ftruncate(FileHandle file, size_t new_size)
{
	int res = ::ftruncate(file, new_size);
	return (res >= 0) ? res : syserr();
}

int FileSystem::flush(FileHandle file)
{
	int res = ::fsync(file);
	return (res >= 0) ? res : syserr();
}

int FileSystem::rename(const char* oldpath, const char* newpath)
{
	int res = ::rename(oldpath, newpath);
	return (res >= 0) ? res : syserr();
}

int FileSystem::remove(const char* path)
{
	int res = ::remove(path);
	return (res >= 0) ? res : syserr();
}

} // namespace Host
} // namespace IFS
