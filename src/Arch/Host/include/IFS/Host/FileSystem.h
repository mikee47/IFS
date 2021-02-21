/*
 * FileSystem.h
 *
 *  Created on: 11 September 2018
 *      Author: mikee47
 *
 * Standard File System
 *
 * IFS wrapper for Host (POSIX) file system
 *
 */

#pragma once

#include <IFS/IFileSystem.h>

struct stat;

namespace IFS
{
namespace Host
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
	int opendir(const char* path, DirHandle& dir) override;
	int rewinddir(DirHandle dir) override;
	int readdir(DirHandle dir, Stat& stat) override;
	int closedir(DirHandle dir) override;
	int mkdir(const char* path) override;
	int stat(const char* path, Stat* stat) override;
	int fstat(FileHandle file, Stat* stat) override;
	int setacl(FileHandle file, const ACL& acl) override;
	int setattr(const char* path, FileAttributes attr) override;
	int settime(FileHandle file, time_t mtime) override;
	int setcompression(FileHandle file, const Compression& compression) override;
	FileHandle open(const char* path, OpenFlags flags) override;
	FileHandle fopen(const Stat& stat, OpenFlags flags) override;
	int close(FileHandle file) override;
	int read(FileHandle file, void* data, size_t size) override;
	int write(FileHandle file, const void* data, size_t size) override;
	int lseek(FileHandle file, int offset, SeekOrigin origin) override;
	int eof(FileHandle file) override;
	int32_t tell(FileHandle file) override;
	int truncate(FileHandle file, size_t new_size) override;
	int flush(FileHandle file) override
	{
		return Error::ReadOnly;
	}
	int rename(const char* oldpath, const char* newpath) override
	{
		return Error::ReadOnly;
	}
	int remove(const char* path) override
	{
		return Error::ReadOnly;
	}
	int fremove(FileHandle file) override
	{
		return Error::ReadOnly;
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
	void fillStat(const struct stat& s, Stat& stat);
};

} // namespace Host
} // namespace IFS
