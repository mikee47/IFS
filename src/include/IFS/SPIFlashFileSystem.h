/*
 * SPIFlashFileSystem.h
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

#include "IFS.h"
#include <spiffs.h>

#define FFS_LOG_PAGE_SIZE 256

/*
 * Maxmimum number of open files
 */
#define FFS_MAX_FILEDESC 8

#pragma pack(1)

/** @brief Content of SPIFFS metadata area
 */
struct FileMeta {
	// Modification time
	time_t mtime;
	// FileAttr - default indicates content has changed
	FileAttributes attr;
	// Used internally, always 0xFF on disk
	uint8_t __flags;
	// Security
	FileACL acl;

	// We use '0' for dirty so when it's clear disk gets a '1', flash default
	void setDirty()
	{
		bitClear(__flags, 0);
	}

	void clearDirty()
	{
		bitSet(__flags, 0);
	}

	bool isDirty()
	{
		return !bitRead(__flags, 0);
	}
};

union SpiffsMetaBuffer {
	uint8_t buffer[SPIFFS_OBJ_META_LEN] = {0};
	FileMeta meta;
};

#pragma pack()

/*
 * Wraps SPIFFS
 */
class SPIFlashFileSystem : public IFileSystem
{
public:
	SPIFlashFileSystem(IFSMedia* media) : _media(media)
	{
	}

	~SPIFlashFileSystem() override;

	int mount() override;
	int getinfo(FileSystemInfo& info) override;
	int geterrortext(int err, char* buffer, size_t size) override;
	int opendir(const char* path, filedir_t* dir) override;
	int readdir(filedir_t dir, FileStat* stat) override;
	int closedir(filedir_t dir) override;
	int stat(const char* path, FileStat* stat) override;
	int fstat(file_t file, FileStat* stat) override;
	int setacl(file_t file, FileACL* acl) override;
	int setattr(file_t file, FileAttributes attr) override;
	int settime(file_t file, time_t mtime) override;
	file_t open(const char* path, FileOpenFlags flags) override;
	file_t fopen(const FileStat& stat, FileOpenFlags flags) override;
	int close(file_t file) override;
	int read(file_t file, void* data, size_t size) override;
	int write(file_t file, const void* data, size_t size) override;
	int lseek(file_t file, int offset, SeekOriginFlags origin) override;
	int eof(file_t file) override;
	int32_t tell(file_t file) override;
	int truncate(file_t file, size_t new_size) override;
	int flush(file_t file) override;
	int rename(const char* oldpath, const char* newpath) override;
	int remove(const char* path) override;
	int fremove(file_t file) override;
	int format() override;
	int check() override;
	int isfile(file_t file) override;

	/** @brief get the full path of a file from its ID
	 *  @param fileid
	 *  @param buffer
	 *  @retval int error code
	 */
	int getFilePath(fileid_t fileid, NameBuffer& buffer);

private:
	spiffs* handle()
	{
		return &_fs;
	}

	int _mount(spiffs_config& cfg);

	/** @brief Pull metadata from SPIFFS
	 *  @param file valid file handle
	 * 	@retval FileMeta& reference to the metadata
	 *  @note Called when file opened
	 */
	SpiffsMetaBuffer* cacheMeta(file_t file);

	int getMeta(file_t file, SpiffsMetaBuffer*& meta);
	void updateMetaCache(file_t file, const spiffs_stat& ss);
	int flushMeta(file_t file);
	void touch(file_t file)
	{
		settime(file, fsGetTimeUTC());
	}

private:
	IFSMedia* _media = nullptr;
	SpiffsMetaBuffer _metaCache[FFS_MAX_FILEDESC];
	spiffs _fs;
	uint8_t* _work_buf = nullptr;
	uint8_t* _fds = nullptr;
	u8_t* _cache = nullptr;
};
