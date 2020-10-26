/*
 * StdFileSystem.h
 *
 *  Created on: 11 September 2018
 *      Author: mikee47
 *
 * Standard File System
 *
 * IFS wrapper for POSIX file system
 *
 */

#pragma once

#include <IFS/FileSystem.h>
#include <sys/stat.h>

namespace IFS
{
/** @brief Implementation of standard mingw filing system using IFS
 */
class StdFileSystem : public IFileSystem
{
public:
	StdFileSystem()
	{
	}

	~StdFileSystem() override
	{
	}

	// IFileSystem methods
	int getinfo(FileSystemInfo& info) override;
	int geterrortext(int err, char* buffer, size_t size) override;
	int opendir(const char* path, filedir_t* dir) override;
	int readdir(filedir_t dir, FileStat* stat) override;
	int closedir(filedir_t dir) override;
	int stat(const char* path, FileStat* stat) override;
	int fstat(file_t file, FileStat* stat) override;
	int setacl(file_t file, FileACL* acl) override
	{
		return Error::ReadOnly;
	}
	int setattr(file_t file, FileAttributes attr) override
	{
		return Error::ReadOnly;
	}
	int settime(file_t file, time_t mtime) override
	{
		return Error::ReadOnly;
	}
	file_t open(const char* path, FileOpenFlags flags) override;
	file_t fopen(const FileStat& stat, FileOpenFlags flags) override;
	int close(file_t file) override;
	int read(file_t file, void* data, size_t size) override;
	int write(file_t file, const void* data, size_t size) override
	{
		return Error::ReadOnly;
	}
	int lseek(file_t file, int offset, SeekOrigin origin) override;
	int eof(file_t file) override;
	int32_t tell(file_t file) override;
	int truncate(file_t file, size_t new_size) override;
	int flush(file_t file) override
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
	int fremove(file_t file) override
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
	int isfile(file_t file) override
	{
		return file >= 0;
	}

private:
	void fillStat(const struct stat& s, FileStat& stat);
};

} // namespace IFS
