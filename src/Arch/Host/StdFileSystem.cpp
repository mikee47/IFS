/*
 * StdFileSystem.cpp
 *
 *  Created on: 11 Sep 2018
 *      Author: Mike
 */

#define _POSIX_C_SOURCE 200112L

#include "include/IFS/StdFileSystem.h"
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

namespace IFS
{
namespace
{
int syserr()
{
	return -errno;
}

int mapFlags(File::OpenFlags flags)
{
	int ret = 0;
	if(flags[File::OpenFlag::Append]) {
		ret |= O_APPEND;
	}
	if(flags[File::OpenFlag::Create]) {
		ret |= O_CREAT;
	}
	if(flags[File::OpenFlag::Read]) {
		ret |= O_RDONLY;
	}
	if(flags[File::OpenFlag::Truncate]) {
		ret |= O_TRUNC;
	}
	if(flags[File::OpenFlag::Write]) {
		ret |= O_WRONLY;
	}
	return ret;
}

} // namespace

struct FileDir {
	char path[PATH_MAX];
	DIR* d;
};

int StdFileSystem::getinfo(Info& info)
{
	return Error::NotImplemented;
}

String StdFileSystem::getErrorString(int err)
{
	char buffer[256];
	auto res = strerror_r(err, buffer, sizeof(buffer));
	if(int(res) <= 0) {
		return F("Unknown");
	}
	return String(buffer);
}

int StdFileSystem::opendir(const char* path, DirHandle& dir)
{
	auto d = new FileDir;

	d->d = ::opendir(path);
	if(d->d != nullptr) {
		strcpy(d->path, path);
		dir = d;
		return FS_OK;
	}

	int res = syserr();
	delete d;
	return res;
}

int StdFileSystem::readdir(DirHandle dir, FileStat& stat)
{
	int res;
	dirent* e = ::readdir(dir->d);
	if(e == nullptr) {
		res = syserr();
	} else {
		char path[PATH_MAX];
		strcpy(path, dir->path);
		strcat(path, "/");
		strcat(path, e->d_name);
		res = this->stat(path, &stat);
#ifdef __WIN32
		if(e->d_type & _A_SUBDIR) {
			stat.attr |= File::Attribute::Directory;
		}
		if(e->d_type & _A_RDONLY) {
			stat.attr |= File::Attribute::ReadOnly;
		}
		if(e->d_type & _A_ARCH) {
			stat.attr |= File::Attribute::Archive;
		}
#endif
		//		stat->name.copy(e->d_name, e->d_namlen);
	}

	return res;
}

int StdFileSystem::closedir(DirHandle dir)
{
	auto d = reinterpret_cast<DIR*>(dir);
	int res = ::closedir(d);
	return res;
}

void StdFileSystem::fillStat(const struct stat& s, FileStat& stat)
{
	stat.clear();
	stat.fs = this;
	if(S_ISDIR(s.st_mode)) {
		stat.attr |= File::Attribute::Directory;
	}
	stat.mtime = s.st_mtime;
}

int StdFileSystem::stat(const char* path, FileStat* stat)
{
	struct stat s;
	int res = ::stat(path, &s);
	if(res >= 0 && stat != nullptr) {
		fillStat(s, *stat);
		const char* lastSep = strrchr(path, '/');
		stat->name.copy(lastSep ? lastSep + 1 : path);
	}

	return res;
}

int StdFileSystem::fstat(File::Handle file, FileStat* stat)
{
	struct stat s;
	int res = ::fstat(file, &s);
	if(res >= 0 && stat) {
		fillStat(s, *stat);
	}

	return res;
}

File::Handle StdFileSystem::open(const char* path, File::OpenFlags flags)
{
	int res = ::open(path, mapFlags(flags));
	return res;
}

File::Handle StdFileSystem::fopen(const FileStat& stat, File::OpenFlags flags)
{
	return Error::NotImplemented;
}

int StdFileSystem::close(File::Handle file)
{
	return ::close(file);
}

int StdFileSystem::read(File::Handle file, void* data, size_t size)
{
	return ::read(file, data, size);
}

int StdFileSystem::lseek(File::Handle file, int offset, File::SeekOrigin origin)
{
	return ::lseek(file, offset, uint8_t(origin));
}

int StdFileSystem::eof(File::Handle file)
{
	// POSIX doesn't appear to have eof()

	int pos = tell(file);
	if(pos < 0) {
		return pos;
	}

	struct stat stat;
	int err = ::fstat(file, &stat);
	if(err < 0) {
		return err;
	}

	return (pos >= stat.st_size) ? 1 : 0;
}

int32_t StdFileSystem::tell(File::Handle file)
{
	return ::lseek(file, 0, SEEK_CUR);
}

int StdFileSystem::truncate(File::Handle file, size_t new_size)
{
	return ::ftruncate(file, new_size);
}

} // namespace IFS
