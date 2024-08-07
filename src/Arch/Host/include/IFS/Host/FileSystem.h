/****
 * FileSystem.h
 * IFS wrapper for Host (POSIX) file system
 *
 * Created on: 11 September 2018
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

#pragma once

#include <IFS/IFileSystem.h>

namespace IFS::Host
{
struct os_stat_t;

/**
 * @brief IFS implementation of Host filing system
 */
class FileSystem : public IFileSystem
{
public:
	FileSystem(const char* rootpath = nullptr) : rootpath(rootpath), mounted(!rootpath)
	{
	}

	~FileSystem() override = default;

	int mount() override;

	// IFileSystem methods
	int getinfo(Info& info) override;
	String getErrorString(int err) override;
	int opendir(const char* path, DirHandle& dir) override;
	int rewinddir(DirHandle dir) override;
	int readdir(DirHandle dir, Stat& stat) override;
	int closedir(DirHandle dir) override;
	int mkdir(const char* path) override;
	int stat(const char* path, Stat* stat) override;
	int fstat(FileHandle file, Stat* stat) override;
	int fsetxattr(FileHandle file, AttributeTag tag, const void* data, size_t size) override;
	int fgetxattr(FileHandle file, AttributeTag tag, void* buffer, size_t size) override;
	int fenumxattr(FileHandle file, AttributeEnumCallback callback, void* buffer, size_t bufsize) override;
	int setxattr(const char* path, AttributeTag tag, const void* data, size_t size) override;
	int getxattr(const char* path, AttributeTag tag, void* buffer, size_t size) override;
	FileHandle open(const char* path, OpenFlags flags) override;
	int close(FileHandle file) override;
	int read(FileHandle file, void* data, size_t size) override;
	int write(FileHandle file, const void* data, size_t size) override;
	file_offset_t lseek(FileHandle file, file_offset_t offset, SeekOrigin origin) override;
	int eof(FileHandle file) override;
	file_offset_t tell(FileHandle file) override;
	int ftruncate(FileHandle file, file_size_t new_size) override;
	int flush(FileHandle file) override;
	int rename(const char* oldpath, const char* newpath) override;
	int remove(const char* path) override;
	int fremove(FileHandle) override
	{
		return Error::NotImplemented;
	}
	int format() override
	{
		return Error::ReadOnly;
	}
	int check() override
	{
		return Error::NotImplemented;
	}

private:
	String resolvePath(const char* path);
	void fillStat(const os_stat_t& s, Stat& stat);
	String rootpath;
	bool mounted;
};

} // namespace IFS::Host
