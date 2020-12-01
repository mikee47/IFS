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

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

namespace IFS
{
struct FileDir {
	char path[PATH_MAX];
	DIR* d;
};

namespace Host
{
int FileSystem::getinfo(Info& info)
{
	return Error::NotImplemented;
}

String FileSystem::getErrorString(int err)
{
	return IFS::Host::getErrorString(err);
}

int FileSystem::opendir(const char* path, DirHandle& dir)
{
	auto d = new FileDir;

	d->d = ::opendir(path);
	if(d->d != nullptr) {
		strcpy(d->path, path);
		dir = d;
		return FS_OK;
	}

	int err = syserr();
	delete d;
	return err;
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

int FileSystem::closedir(DirHandle dir)
{
	auto d = reinterpret_cast<DIR*>(dir);
	int res = ::closedir(d);
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
	stat.clear();
	stat.fs = this;
	if(S_ISDIR(s.st_mode)) {
		stat.attr |= File::Attribute::Directory;
	}
	stat.mtime = s.st_mtime;
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
	}

	return res;
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
	}

	return res;
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
