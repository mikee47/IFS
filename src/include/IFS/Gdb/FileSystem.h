/*
 * FileSystem.h
 *
 *  Created on: 1 December 2020
 *      Author: mikee47
 *
 * IFS wrapper for GDB syscall file access
 *
 */

#pragma once

#include <IFS/IFileSystem.h>

namespace IFS
{
namespace Gdb
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
	int readdir(DirHandle dir, FileStat& stat) override
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
	int stat(const char* path, FileStat* stat) override;
	int fstat(File::Handle file, FileStat* stat) override;
	int setacl(File::Handle file, const File::ACL& acl) override
	{
		return Error::NotSupported;
	}
	int setattr(const char* filename, File::Attributes attr) override
	{
		return Error::NotSupported;
	}
	int settime(File::Handle file, time_t mtime) override
	{
		return Error::NotSupported;
	}
	int setcompression(File::Handle file, const File::Compression& compression) override
	{
		return Error::NotSupported;
	}
	File::Handle open(const char* path, File::OpenFlags flags) override;
	File::Handle fopen(const FileStat& stat, File::OpenFlags flags) override
	{
		return Error::NotSupported;
	}
	int close(File::Handle file) override;
	int read(File::Handle file, void* data, size_t size) override;
	int write(File::Handle file, const void* data, size_t size) override;
	int lseek(File::Handle file, int offset, SeekOrigin origin) override;
	int eof(File::Handle file) override;
	int32_t tell(File::Handle file) override;
	int truncate(File::Handle file, size_t new_size) override
	{
		return Error::NotSupported;
	}
	int flush(File::Handle file) override
	{
		return Error::NotSupported;
	}
	int rename(const char* oldpath, const char* newpath) override;
	int remove(const char* path) override;
	int fremove(File::Handle file) override
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

} // namespace Gdb
} // namespace IFS
