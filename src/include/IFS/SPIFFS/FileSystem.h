/*
 * FileSystem.h
 *
 *  Created on: 21 Jul 2018
 *      Author: mikee47
 *
 * Provides an IFS FileSystem implementation for SPIFFS.
 *
 * This is mostly a straightforward wrapper around SPIFFS, with a few enhancements:
 *
 *	Metadata caching
 *
 *		Metadata can be updated multiple times while a file is open so
 * 		for efficiency it is kept in RAM and only written to SPIFFS on close or flush.
 *
 *	Directory emulation
 *
 *		SPIFFS stores all files in a flat format, so directory behaviour is emulated
 *		including opendir() and readdir() operations. Overall path length is fixed
 *		according to SPIFFS_OBJ_NAME_LEN.
 *
 *  File truncation
 *
 *  	Standard IFS truncate() method allows file size to be reduced.
 *  	This was added to Sming in version 4.
 *
 */

#pragma once

#include "../FileSystem.h"
#include "FileMeta.h"
#include <spiffs.h>
extern "C" {
#include <spiffs_nucleus.h>
}

/*
 * Maxmimum number of open files
 */
#define FFS_MAX_FILEDESC 8

namespace IFS
{
namespace SPIFFS
{
/*
 * Wraps SPIFFS
 */
class FileSystem : public IFileSystem
{
public:
	FileSystem(Storage::Partition partition) : partition(partition)
	{
	}

	~FileSystem();

	int mount() override;
	int getinfo(Info& info) override;
	String getErrorString(int err) override;
	int opendir(const char* path, DirHandle& dir) override;
	int readdir(DirHandle dir, Stat& stat) override;
	int rewinddir(DirHandle dir) override
	{
		return Error::NotSupported;
	}
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
	int flush(FileHandle file) override;
	int rename(const char* oldpath, const char* newpath) override;
	int remove(const char* path) override;
	int fremove(FileHandle file) override;
	int format() override;
	int check() override;

	/** @brief get the full path of a file from its ID
	 *  @param fileid
	 *  @param buffer
	 *  @retval int error code
	 */
	int getFilePath(FileID fileid, NameBuffer& buffer);

private:
	spiffs* handle()
	{
		return &fs;
	}

	int tryMount(spiffs_config& cfg);

	SpiffsMetaBuffer* initMetaBuffer(FileHandle file);
	SpiffsMetaBuffer* getMetaBuffer(FileHandle file);
	int flushMeta(FileHandle file);

	void touch(FileHandle file)
	{
		settime(file, fsGetTimeUTC());
	}

	static constexpr size_t CACHE_PAGES{8};
	static constexpr size_t LOG_PAGE_SIZE{256};
	static constexpr size_t MIN_BLOCKSIZE{256};
	static constexpr size_t CACHE_PAGE_SIZE{sizeof(spiffs_cache_page) + LOG_PAGE_SIZE};
	static constexpr size_t CACHE_SIZE{sizeof(spiffs_cache) + CACHE_PAGES * CACHE_PAGE_SIZE};

	Storage::Partition partition;
	SpiffsMetaBuffer metaCache[FFS_MAX_FILEDESC];
	spiffs fs;
	uint16_t workBuffer[LOG_PAGE_SIZE];
	spiffs_fd fileDescriptors[FFS_MAX_FILEDESC];
	uint8_t cache[CACHE_SIZE];
};

} // namespace SPIFFS
} // namespace IFS
