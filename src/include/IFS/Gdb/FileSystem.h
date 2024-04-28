/****
 * FileSystem.h
 * IFS wrapper for GDB syscall file access
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

#pragma once

#include "../IFileSystem.h"

namespace IFS::Gdb
{
/**
 * @brief IFS implementation of Host filing system
 */
class FileSystem : public IFileSystem
{
public:
	FileSystem()
	{
	}

	~FileSystem() override
	{
	}

	int mount() override
	{
		return FS_OK;
	}

	// IFileSystem methods
	int getinfo(Info& info) override;
	String getErrorString(int err) override;
	int opendir(const char* path, DirHandle& dir) override
	{
		return Error::NotSupported;
	}
	int rewinddir(DirHandle dir) override
	{
		return Error::NotSupported;
	}
	int readdir(DirHandle dir, Stat& stat) override
	{
		return Error::NotSupported;
	}
	int closedir(DirHandle dir) override
	{
		return Error::NotSupported;
	}
	int mkdir(const char* path) override
	{
		return Error::NotSupported;
	}
	int stat(const char* path, Stat* stat) override;
	int fstat(FileHandle file, Stat* stat) override;
	int fsetxattr(FileHandle file, AttributeTag tag, const void* data, size_t size) override
	{
		return Error::NotSupported;
	}
	int fgetxattr(FileHandle file, AttributeTag tag, void* buffer, size_t size) override
	{
		return Error::NotSupported;
	}
	int fenumxattr(FileHandle file, AttributeEnumCallback callback, void* buffer, size_t bufsize) override
	{
		return Error::NotSupported;
	}
	int setxattr(const char* path, AttributeTag tag, const void* data, size_t size) override
	{
		return Error::NotSupported;
	}
	int getxattr(const char* path, AttributeTag tag, void* buffer, size_t size) override
	{
		return Error::NotSupported;
	}
	FileHandle open(const char* path, OpenFlags flags) override;
	int close(FileHandle file) override;
	int read(FileHandle file, void* data, size_t size) override;
	int write(FileHandle file, const void* data, size_t size) override;
	file_offset_t lseek(FileHandle file, file_offset_t offset, SeekOrigin origin) override;
	int eof(FileHandle file) override;
	file_offset_t tell(FileHandle file) override;
	int ftruncate(FileHandle file, file_size_t new_size) override
	{
		return Error::NotSupported;
	}
	int flush(FileHandle file) override
	{
		return Error::NotSupported;
	}
	int rename(const char* oldpath, const char* newpath) override;
	int remove(const char* path) override;
	int fremove(FileHandle file) override
	{
		return Error::NotImplemented;
	}
	int format() override
	{
		return Error::NotSupported;
	}
	int check() override
	{
		return FS_OK;
	}
};

} // namespace IFS::Gdb
