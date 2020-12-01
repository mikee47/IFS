/*
 * GdbFileSystem.cpp
 *
 *  Created on: 1 December 2020
 *      Author: Mike
 */

#include "include/IFS/Gdb/FileSystem.h"
#include <IFS/Host/Util.h>
#include <gdb/gdb_syscall.h>

namespace IFS
{
namespace Gdb
{
namespace
{
void fillStat(IFileSystem* fs, const gdb_stat_t& s, FileStat& stat)
{
	stat.clear();
	stat.fs = fs;
	if(S_ISDIR(s.st_mode)) {
		stat.attr |= File::Attribute::Directory;
	}
	stat.id = s.st_ino;
	stat.mtime = s.st_mtime;
}

} // namespace

int FileSystem::getinfo(Info& info)
{
	return Error::NotImplemented;
}

String FileSystem::getErrorString(int err)
{
	return IFS::Host::getErrorString(err);
}

int FileSystem::stat(const char* path, FileStat* stat)
{
	gdb_stat_t s;
	int res = gdb_syscall_stat(path, &s);
	if(res < 0) {
		return IFS::Host::syserr();
	}

	if(stat != nullptr) {
		fillStat(this, s, *stat);
		const char* lastSep = strrchr(path, '/');
		stat->name.copy(lastSep ? lastSep + 1 : path);
	}

	return res;
}

int FileSystem::fstat(File::Handle file, FileStat* stat)
{
	gdb_stat_t s;
	int res = gdb_syscall_fstat(file, &s);
	if(res < 0) {
		return IFS::Host::syserr();
	}

	if(stat != nullptr) {
		fillStat(this, s, *stat);
	}

	return res;
}

File::Handle FileSystem::open(const char* path, File::OpenFlags flags)
{
	int res = gdb_syscall_open(path, IFS::Host::mapFlags(flags), S_IRWXU);
	assert(File::Handle(res) == res);
	return (res >= 0) ? res : IFS::Host::syserr();
}

int FileSystem::close(File::Handle file)
{
	int res = gdb_syscall_close(file);
	return (res >= 0) ? res : IFS::Host::syserr();
}

int FileSystem::read(File::Handle file, void* data, size_t size)
{
	int res = gdb_syscall_read(file, data, size);
	return (res >= 0) ? res : IFS::Host::syserr();
}

int FileSystem::write(File::Handle file, const void* data, size_t size)
{
	int res = gdb_syscall_write(file, data, size);
	return (res >= 0) ? res : IFS::Host::syserr();
}

int FileSystem::lseek(File::Handle file, int offset, SeekOrigin origin)
{
	int res = gdb_syscall_lseek(file, offset, uint8_t(origin));
	return (res >= 0) ? res : IFS::Host::syserr();
}

int FileSystem::eof(File::Handle file)
{
	// POSIX doesn't appear to have eof()

	int pos = tell(file);
	if(pos < 0) {
		return IFS::Host::syserr();
	}

	gdb_stat_t stat;
	int err = gdb_syscall_fstat(file, &stat);
	if(err < 0) {
		return IFS::Host::syserr();
	}

	return (uint32_t(pos) >= stat.st_size) ? 1 : 0;
}

int32_t FileSystem::tell(File::Handle file)
{
	return lseek(file, 0, SeekOrigin::Current);
}

int FileSystem::rename(const char* oldpath, const char* newpath)
{
	int res = gdb_syscall_rename(oldpath, newpath);
	return (res >= 0) ? res : IFS::Host::syserr();
}

int FileSystem::remove(const char* path)
{
	int res = gdb_syscall_unlink(path);
	return (res >= 0) ? res : IFS::Host::syserr();
}

} // namespace Gdb
} // namespace IFS
