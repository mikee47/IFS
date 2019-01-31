/*
 * StdFileSystem.cpp
 *
 *  Created on: 11 Sep 2018
 *      Author: Mike
 */

#include "StdFileSystem.h"

#include <io.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#define PATH_MAX 255

struct FileDir {
	char path[PATH_MAX];
	DIR* d;
};

int syserr()
{
	return -errno;
}

int StdFileSystem::getinfo(FileSystemInfo& info)
{
	return FSERR_NotImplemented;
}

int StdFileSystem::geterrortext(int err, char* buffer, size_t size)
{
	return strerror_r(err, buffer, size);
}

int StdFileSystem::opendir(const char* path, filedir_t* dir)
{
	auto d = new FileDir;

	d->d = ::opendir(path);
	if(d) {
		strcpy(d->path, path);
		*dir = reinterpret_cast<filedir_t>(d);
		return FS_OK;
	}

	int res = syserr();
	delete d;
	return res;
}

int StdFileSystem::readdir(filedir_t dir, FileStat* stat)
{
	int res;
	dirent* e = ::readdir(dir->d);
	if(!e)
		res = syserr();
	else if(stat) {
		char path[PATH_MAX];
		strcpy(path, dir->path);
		strcat(path, "/");
		strcat(path, e->d_name);
		res = this->stat(path, stat);
		if(e->d_type & _A_SUBDIR)
			bitSet(stat->attr, FileAttr::Directory);
		if(e->d_type & _A_RDONLY)
			bitSet(stat->attr, FileAttr::ReadOnly);
		if(e->d_type & _A_ARCH)
			bitSet(stat->attr, FileAttr::Archive);
		//		stat->name.copy(e->d_name, e->d_namlen);
	}

	return res;
}

int StdFileSystem::closedir(filedir_t dir)
{
	auto d = reinterpret_cast<DIR*>(dir);
	int res = ::closedir(d);
	return res;
}

void StdFileSystem::fillStat(const struct stat& s, FileStat& stat)
{
	stat.clear();
	stat.fs = this;
	if(S_ISDIR(s.st_mode))
		bitSet(stat.attr, FileAttr::Directory);
	stat.mtime = s.st_mtime;
}

int StdFileSystem::stat(const char* path, FileStat* stat)
{
	struct stat s;
	int res = ::stat(path, &s);
	if(res >= 0 && stat) {
		fillStat(s, *stat);
		const char* lastSep = strrchr(path, '/');
		stat->name.copy(lastSep ? lastSep + 1 : path);
	}

	return res;
}

int StdFileSystem::fstat(file_t file, FileStat* stat)
{
	struct stat s;
	int res = ::fstat(file, &s);
	if(res >= 0 && stat) {
		fillStat(s, *stat);
	}

	return res;
}

int mapFlags(FileOpenFlags flags)
{
	int ret = 0;
	if(bitRead(flags, FileOpenFlag::Append))
		ret |= O_APPEND;
	if(bitRead(flags, FileOpenFlag::Create))
		ret |= O_CREAT;
	if(bitRead(flags, FileOpenFlag::Read))
		ret |= O_RDONLY;
	if(bitRead(flags, FileOpenFlag::Truncate))
		ret |= O_TRUNC;
	if(bitRead(flags, FileOpenFlag::Write))
		ret |= O_WRONLY;
	return ret;
}

file_t StdFileSystem::open(const char* path, FileOpenFlags flags)
{
	int res = ::open(path, mapFlags(flags));
	return res;
}

file_t StdFileSystem::fopen(const FileStat& stat, FileOpenFlags flags)
{
	return FSERR_NotImplemented;
}

int StdFileSystem::close(file_t file)
{
	return ::close(file);
}

int StdFileSystem::read(file_t file, void* data, size_t size)
{
	return ::read(file, data, size);
}

int StdFileSystem::lseek(file_t file, int offset, SeekOriginFlags origin)
{
	return ::lseek(file, offset, origin);
}

int StdFileSystem::eof(file_t file)
{
	return ::eof(file);
}

int32_t StdFileSystem::tell(file_t file)
{
	return ::tell(file);
}

int StdFileSystem::isfile(file_t file)
{
	return file >= 0;
}

int StdFileSystem::getFilePath(fileid_t fileid, NameBuffer& path)
{
	return FSERR_NotImplemented;
}
