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
	int readdir(DirHandle dir, FileStat& stat) override;
	int rewinddir(DirHandle dir) override
	{
		return Error::NotSupported;
	}
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
	int flush(File::Handle file) override;
	int rename(const char* oldpath, const char* newpath) override;
	int remove(const char* path) override;
	int fremove(File::Handle file) override;
	int format() override;
	int check() override;

	/** @brief get the full path of a file from its ID
	 *  @param fileid
	 *  @param buffer
	 *  @retval int error code
	 */
	int getFilePath(File::ID fileid, NameBuffer& buffer);

private:
	spiffs* handle()
	{
		return &fs;
	}

	int tryMount(spiffs_config& cfg);

	SpiffsMetaBuffer* initMetaBuffer(File::Handle file);
	SpiffsMetaBuffer* getMetaBuffer(File::Handle file);
	int flushMeta(File::Handle file);

	void touch(File::Handle file)
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
