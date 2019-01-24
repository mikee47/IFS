/*
 * SPIFlashFileSystem.h
 *
 *  Created on: 21 Jul 2018
 *      Author: mikee47
 *
 * Provides an IFS FileSystem implementation for SPIFFS.
 *
 * This is pretty much a straightforward wrapper around SPIFFS, with the
 * addition of metadata caching. Metadata can be updated multiple
 * times while a file is open so for efficiency it is kept in RAM and
 * only written to SPIFFS on close or flush.
 *
 */

#ifndef __SPIFLASH_FILESYSTEM_H
#define __SPIFLASH_FILESYSTEM_H

#include "IFS.h"
#include "spiffs.h"

#define FFS_LOG_PAGE_SIZE 256

/*
 * Maxmimum number of open files
 */
#define FFS_MAX_FILEDESC 8

/** @brief Underlying file system stores information in meta area laid out in this structure.
 */
struct __packed FileMeta {
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

/*
 * Wraps SPIFFS
 */
class SPIFlashFileSystem : public IFileSystem
{
public:
	SPIFlashFileSystem(IFSMedia* media) : _media(media)
	{
	}

	virtual ~SPIFlashFileSystem();

	int mount();
	virtual int getinfo(FileSystemInfo& info);
	virtual int geterrortext(int err, char* buffer, size_t size);
	virtual int opendir(const char* path, filedir_t* dir);
	virtual int readdir(filedir_t dir, FileStat* stat);
	virtual int closedir(filedir_t dir);
	virtual int stat(const char* path, FileStat* stat);
	virtual int fstat(file_t file, FileStat* stat);
	virtual int setacl(file_t file, FileACL* acl);
	virtual int setattr(file_t file, FileAttributes attr);
	virtual int settime(file_t file, time_t mtime);
	virtual file_t open(const char* path, FileOpenFlags flags);
	virtual file_t fopen(const FileStat& stat, FileOpenFlags flags);
	virtual int close(file_t file);
	virtual int read(file_t file, void* data, size_t size);
	virtual int write(file_t file, const void* data, size_t size);
	virtual int lseek(file_t file, int offset, SeekOriginFlags origin);
	virtual int eof(file_t file);
	virtual int32_t tell(file_t file);
	virtual int truncate(file_t file);
	virtual int flush(file_t file);
	virtual int rename(const char* oldpath, const char* newpath);
	virtual int remove(const char* path);
	virtual int fremove(file_t file);
	virtual int format();
	virtual int check();
	virtual int isfile(file_t file);

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

#endif // __SPIFLASH_FILESYSTEM_H
