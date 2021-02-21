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
	int readdir(DirHandle dir, FileStat& stat) override;
	int closedir(DirHandle dir) override;
	int mkdir(const char* path) override;
	int stat(const char* path, FileStat* stat) override;
	int fstat(File::Handle file, FileStat* stat) override;
	int setacl(File::Handle file, const File::ACL& acl) override;
	int setattr(const char* path, File::Attributes attr) override;
	int settime(File::Handle file, time_t mtime) override;
	int setcompression(File::Handle file, const File::Compression& compression) override;
	File::Handle open(const char* path, File::OpenFlags flags) override;
	File::Handle fopen(const FileStat& stat, File::OpenFlags flags) override;
	int close(File::Handle file) override;
	int read(File::Handle file, void* data, size_t size) override;
	int write(File::Handle file, const void* data, size_t size) override;
	int lseek(File::Handle file, int offset, SeekOrigin origin) override;
	int eof(File::Handle file) override;
	int32_t tell(File::Handle file) override;
	int truncate(File::Handle file, size_t new_size) override;
	int flush(File::Handle file) override
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
	int fremove(File::Handle file) override
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
	void fillStat(const struct stat& s, FileStat& stat);
};

} // namespace Host
} // namespace IFS
