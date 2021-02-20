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

namespace Host
{
namespace
{
IFS::Host::FileSystem hostFileSystem;

const char* extendedAttributeName{"user.sming.ifs"};

/**
 * @brief Stored with file
 */
struct ExtendedAttributes {
	uint32_t size; ///< Size of this structure
	File::ACL acl;
	File::Attributes attr;
	File::Compression compression;
};

int setExtendedAttributes(File::Handle file, const ExtendedAttributes& ea)
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

int getExtendedAttributes(File::Handle file, ExtendedAttributes& ea)
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

int getUserAttributes(File::Handle file, FileStat& stat)
{
	ExtendedAttributes ea{};

#ifdef __WIN32
	uint32_t attr{0};
	if(fgetattr(file, attr) == 0) {
		stat.attr[File::Attribute::ReadOnly] = (attr & _A_RDONLY);
		stat.attr[File::Attribute::Archive] = (attr & _A_ARCH);
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

int getUserAttributes(const char* path, FileStat& stat)
{
	auto f = fileSystem.open(path, File::ReadOnly);
	if(f < 0) {
		return f;
	}

	int res = getUserAttributes(f, stat);
	if(!stat.attr[File::Attribute::Compressed]) {
		stat.compression.originalSize = stat.size;
	}

	fileSystem.close(f);
	return res;
}

} // namespace

IFileSystem& fileSystem{hostFileSystem};

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

int FileSystem::readdir(DirHandle dir, FileStat& stat)
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

void FileSystem::fillStat(const struct stat& s, FileStat& stat)
{
	stat = FileStat{};
	stat.fs = this;
	stat.id = s.st_ino;
	if((s.st_mode & S_IWUSR) == 0) {
		stat.attr |= File::Attribute::ReadOnly;
	}
	if(S_ISDIR(s.st_mode)) {
		stat.attr |= File::Attribute::Directory;
	}
	stat.mtime = s.st_mtime;
	stat.size = s.st_size;
}

int FileSystem::stat(const char* path, FileStat* stat)
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

int FileSystem::fstat(File::Handle file, FileStat* stat)
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

int FileSystem::setacl(File::Handle file, const File::ACL& acl)
{
	ExtendedAttributes ea{};
	getExtendedAttributes(file, ea);
	ea.acl = acl;
	return setExtendedAttributes(file, ea);
}

int FileSystem::setattr(const char* path, File::Attributes attr)
{
	int file = ::open(path, O_RDWR);
	if(file < 0) {
		return syserr();
	}

	ExtendedAttributes ea{};
	getExtendedAttributes(file, ea);

	constexpr File::Attributes mask{File::Attribute::ReadOnly | File::Attribute::Archive};
	ea.attr -= mask;
	ea.attr += attr & mask;
	int res = setExtendedAttributes(file, ea);
	::close(file);

	return res;
}

int FileSystem::settime(File::Handle file, time_t mtime)
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

int FileSystem::setcompression(File::Handle file, const File::Compression& compression)
{
	ExtendedAttributes ea{};
	getExtendedAttributes(file, ea);
	ea.compression = compression;
	return setExtendedAttributes(file, ea);
}

File::Handle FileSystem::open(const char* path, File::OpenFlags flags)
{
	int res = ::open(path, mapFlags(flags), 0644);
	assert(File::Handle(res) == res);
	return (res >= 0) ? res : syserr();
}

File::Handle FileSystem::fopen(const FileStat& stat, File::OpenFlags flags)
{
	return Error::NotImplemented;
}

int FileSystem::close(File::Handle file)
{
	int res = ::close(file);
	return (res >= 0) ? res : syserr();
}

int FileSystem::read(File::Handle file, void* data, size_t size)
{
	int res = ::read(file, data, size);
	return (res >= 0) ? res : syserr();
}

int FileSystem::write(File::Handle file, const void* data, size_t size)
{
	int res = ::write(file, data, size);
	return (res >= 0) ? res : syserr();
}

int FileSystem::lseek(File::Handle file, int offset, SeekOrigin origin)
{
	int res = ::lseek(file, offset, uint8_t(origin));
	return (res >= 0) ? res : syserr();
}

int FileSystem::eof(File::Handle file)
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

int32_t FileSystem::tell(File::Handle file)
{
	return lseek(file, 0, SeekOrigin::Current);
}

int FileSystem::truncate(File::Handle file, size_t new_size)
{
	int res = ::ftruncate(file, new_size);
	return (res >= 0) ? res : syserr();
}

} // namespace Host
} // namespace IFS
