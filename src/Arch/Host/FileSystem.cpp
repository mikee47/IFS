/*
 * FileSystem.cpp
 *
 *  Created on: 11 Sep 2018
 *      Author: Mike
 */

#define _POSIX_C_SOURCE 200112L

#include <IFS/Host/FileSystem.h>
#include <IFS/Host/Util.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef __WIN32
#include <sys/utime.h>
#include "Windows/xattr.h"
#else
#include <sys/xattr.h>
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
struct FileDir {
	char path[PATH_MAX + 1];
	DIR* d;
};

class FileSystem;

namespace Host
{
namespace
{
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
};

int setExtendedAttributes(FileHandle file, const ExtendedAttributes& ea)
{
	ExtendedAttributes buf{ea};
	buf.size = sizeof(ea);
#if defined(__MACOS)
	int res = fsetxattr(file, extendedAttributeName, &buf, sizeof(buf), 0, 0);
#else
	int res = fsetxattr(file, extendedAttributeName, &buf, sizeof(buf), 0);
#endif
	return (res == 0) ? FS_OK : syserr();
}

int getExtendedAttributes(FileHandle file, ExtendedAttributes& ea)
{
#if defined(__MACOS)
	auto len = fgetxattr(file, extendedAttributeName, &ea, sizeof(ea), 0, 0);
#else
	auto len = fgetxattr(file, extendedAttributeName, &ea, sizeof(ea));
#endif
	if(len < 0) {
		return syserr();
	}
	if(len != sizeof(ea) || ea.size != sizeof(ea)) {
		return Error::ReadFailure;
	}
	return FS_OK;
}

int getUserAttributes(FileHandle file, Stat& stat)
{
	ExtendedAttributes ea{};

#ifdef __WIN32
	uint32_t attr{0};
	if(fgetattr(file, attr) == 0) {
		stat.attr[FileAttribute::ReadOnly] = (attr & _A_RDONLY);
		stat.attr[FileAttribute::Archive] = (attr & _A_ARCH);
	}
#endif

	int res = getExtendedAttributes(file, ea);
	if(res < 0) {
		return res;
	}

	stat.acl = ea.acl;
	stat.compression = ea.compression;
	stat.attr = ea.attr;

	return FS_OK;
}

int getUserAttributes(const char* path, Stat& stat)
{
	auto f = hostFileSystem.open(path, OpenFlag::Read);
	if(f < 0) {
		return f;
	}

	int res = getUserAttributes(f, stat);
	if(!stat.attr[FileAttribute::Compressed]) {
		stat.compression.originalSize = stat.size;
	}

	hostFileSystem.close(f);
	return res;
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
	assert(strlen(path) < PATH_MAX);

	auto d = new FileDir;

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

	strcpy(d->path, path);
	dir = d;
	return FS_OK;
}

int FileSystem::rewinddir(DirHandle dir)
{
	if(dir->d == nullptr) {
		return Error::InvalidHandle;
	}

	::rewinddir(dir->d);
	return FS_OK;
}

int FileSystem::readdir(DirHandle dir, Stat& stat)
{
	while(true) {
		errno = 0;
		dirent* e = ::readdir(dir->d);
		if(e == nullptr) {
			return syserr() ?: Error::NoMoreFiles;
		}

		if(strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
			continue;
		}

		char path[PATH_MAX];
		strcpy(path, dir->path);
		strcat(path, "/");
		strcat(path, e->d_name);
		int res = this->stat(path, &stat);
		return res;
	}
}

int FileSystem::closedir(DirHandle dir)
{
	int res = ::closedir(dir->d);
	return (res >= 0) ? res : syserr();
}

int FileSystem::mkdir(const char* path)
{
#ifdef __linux__
	int res = ::mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
	int res = ::mkdir(path);
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
		getUserAttributes(path, *stat);
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
		getUserAttributes(file, *stat);
	}

	return res;
}

int FileSystem::setacl(FileHandle file, const ACL& acl)
{
	ExtendedAttributes ea{};
	getExtendedAttributes(file, ea);
	ea.acl = acl;
	return setExtendedAttributes(file, ea);
}

int FileSystem::setattr(const char* path, FileAttributes attr)
{
	int file = ::open(path, O_RDWR);
	if(file < 0) {
		return syserr();
	}

	ExtendedAttributes ea{};
	getExtendedAttributes(file, ea);

	constexpr FileAttributes mask{FileAttribute::ReadOnly + FileAttribute::Archive};
	ea.attr -= mask;
	ea.attr += attr & mask;
	int res = setExtendedAttributes(file, ea);
	::close(file);

	return res;
}

int FileSystem::settime(FileHandle file, time_t mtime)
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

int FileSystem::setcompression(FileHandle file, const IFS::Compression& compression)
{
	ExtendedAttributes ea{};
	getExtendedAttributes(file, ea);
	ea.compression = compression;
	return setExtendedAttributes(file, ea);
}

FileHandle FileSystem::open(const char* path, OpenFlags flags)
{
	int res = ::open(path, mapFlags(flags), 0644);
	assert(FileHandle(res) == res);
	return (res >= 0) ? res : syserr();
}

FileHandle FileSystem::fopen(const Stat& stat, OpenFlags flags)
{
	return Error::NotImplemented;
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

int FileSystem::truncate(FileHandle file, size_t new_size)
{
	int res = ::ftruncate(file, new_size);
	return (res >= 0) ? res : syserr();
}

} // namespace Host
} // namespace IFS
