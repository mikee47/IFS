/**
 * GdbFileSystem.cpp
 *
 * Created on: 1 December 2020
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

#include "include/IFS/Gdb/FileSystem.h"
#include <IFS/Host/Util.h>
#include <gdb/gdb_syscall.h>

namespace IFS
{
namespace Gdb
{
namespace
{
void fillStat(IFileSystem* fs, const gdb_stat_t& s, Stat& stat)
{
	stat = Stat{};
	stat.fs = fs;
	if(S_ISDIR(s.st_mode)) {
		stat.attr |= FileAttribute::Directory;
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

int FileSystem::stat(const char* path, Stat* stat)
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

int FileSystem::fstat(FileHandle file, Stat* stat)
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

FileHandle FileSystem::open(const char* path, IFS::OpenFlags flags)
{
	int res = gdb_syscall_open(path, IFS::Host::mapFlags(flags), S_IRWXU);
	assert(FileHandle(res) == res);
	return (res >= 0) ? res : IFS::Host::syserr();
}

int FileSystem::close(FileHandle file)
{
	int res = gdb_syscall_close(file);
	return (res >= 0) ? res : IFS::Host::syserr();
}

int FileSystem::read(FileHandle file, void* data, size_t size)
{
	int res = gdb_syscall_read(file, data, size);
	return (res >= 0) ? res : IFS::Host::syserr();
}

int FileSystem::write(FileHandle file, const void* data, size_t size)
{
	int res = gdb_syscall_write(file, data, size);
	return (res >= 0) ? res : IFS::Host::syserr();
}

file_offset_t FileSystem::lseek(FileHandle file, file_offset_t offset, SeekOrigin origin)
{
	int res = gdb_syscall_lseek(file, offset, uint8_t(origin));
	return (res >= 0) ? res : IFS::Host::syserr();
}

int FileSystem::eof(FileHandle file)
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

file_offset_t FileSystem::tell(FileHandle file)
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
