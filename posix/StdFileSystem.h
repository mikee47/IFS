/*
 * StdFileSystem.h
 *
 *  Created on: 11 September 2018
 *      Author: mikee47
 *
 * Standard File System
 *
 * IFS wrapper for mingw file system
 *
 */

#ifndef __STD_FILESYSTEM_H
#define __STD_FILESYSTEM_H

#include "IFS/IFS.h"

/** @brief Implementation of standard mingw filing system using IFS
 */
class StdFileSystem : public IFileSystem
{
public:
	StdFileSystem()
	{
	}

	virtual ~StdFileSystem()
	{
	}

	// IFileSystem methods
	virtual int getinfo(FileSystemInfo& info);
	virtual int geterrortext(int err, char* buffer, size_t size);
	virtual int opendir(const char* path, filedir_t* dir);
	virtual int readdir(filedir_t dir, FileStat* stat);
	virtual int closedir(filedir_t dir);
	virtual int stat(const char* path, FileStat* stat);
	virtual int fstat(file_t file, FileStat* stat);
	virtual int setacl(file_t file, FileACL* acl)
	{
		return FSERR_ReadOnly;
	}
	virtual int setattr(file_t file, FileAttributes attr)
	{
		return FSERR_ReadOnly;
	}
	virtual int settime(file_t file, time_t mtime)
	{
		return FSERR_ReadOnly;
	}
	virtual file_t open(const char* path, FileOpenFlags flags);
	virtual file_t fopen(const FileStat& stat, FileOpenFlags flags);
	virtual int close(file_t file);
	virtual int read(file_t file, void* data, size_t size);
	virtual int write(file_t file, const void* data, size_t size)
	{
		return FSERR_ReadOnly;
	}
	virtual int lseek(file_t file, int offset, SeekOriginFlags origin);
	virtual int eof(file_t file);
	virtual int32_t tell(file_t file);
	virtual int truncate(file_t file)
	{
		return FSERR_ReadOnly;
	}
	virtual int flush(file_t file)
	{
		return FSERR_ReadOnly;
	}
	virtual int rename(const char* oldpath, const char* newpath)
	{
		return FSERR_ReadOnly;
	}
	virtual int remove(const char* path)
	{
		return FSERR_ReadOnly;
	}
	virtual int fremove(file_t file)
	{
		return FSERR_ReadOnly;
	}
	virtual int format()
	{
		return FSERR_ReadOnly;
	}
	virtual int check()
	{
		/* We could implement this, but since problems would indicate corrupted firmware
    	 * there isn't much we can do other than suggest a re-flashing. This sort of issue
    	 * is better resolved externally using a hash of the entire firmware image. */
		return FSERR_NotImplemented;
	}
	virtual int isfile(file_t file);

	/** @brief get the full path of a file from its ID
	 *  @param fileid
	 *  @param path
	 *  @retval int error code
	 */
	int getFilePath(fileid_t fileid, NameBuffer& path);

private:
	void fillStat(const struct stat& s, FileStat& stat);
};

#endif // __FIRMWARE_FILESYSTEM_H
