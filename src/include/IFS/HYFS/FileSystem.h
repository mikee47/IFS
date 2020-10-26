/*
 * HybridFileSystem.h
 *
 *  Created on: 22 Jul 2018
 *      Author: mikee47
 *
 * Hybrid file system which layers SPIFFS over FW.
 * Files on SPIFFS are used in preference to FW.
 *
 * Formatting this system wipes SPIFFS to reset to a 'factory default' state.
 *
 * Images are created using a python script.
 *
 */

/* @todo
 * 	When a file is deleted, if it exists on FW then a zero-length file should be placed on SPIFFS
 * 	with an attribute to indicate it has been deleted.
 * 	An 'undelete' could be added which would remove this to restore the FW file. It probably wouldn't
 * 	be useful on any other filing system though, and may not always succeed.
 *
 */

#pragma once

#include "../FWFS/FileSystem.h"
#include "../SPIFFS/FileSystem.h"
#include "../Types.h"

#ifndef HYFS_HIDE_FLAGS
#define HYFS_HIDE_FLAGS 1
#endif

#if HYFS_HIDE_FLAGS == 1
#include "WVector.h"
#endif

namespace IFS
{
namespace HYFS
{
class FileSystem : public IFileSystem
{
public:
	FileSystem(IObjectStore* fwStore, Media* ffsMedia) : fwfs(fwStore), ffs(ffsMedia)
	{
	}

	// IFileSystem methods
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
	int lseek(file_t file, int offset, SeekOrigin origin) override;
	int eof(file_t file) override;
	int32_t tell(file_t file) override;
	int truncate(file_t file, size_t new_size) override;
	int flush(file_t file) override;
	int rename(const char* oldpath, const char* newpath) override;
	int remove(const char* path) override;
	int fremove(file_t file) override;
	int format() override;
	int check() override;
	int isfile(file_t file) override
	{
		int res = fwfs.isfile(file);
		if(res != FS_OK) {
			res = ffs.isfile(file);
		}
		return res;
	}

private:
	int hideFWFile(const char* path, bool hide);
	bool isFWFileHidden(const FileStat& fwstat);

private:
	FWFS::FileSystem fwfs;
	SPIFFS::FileSystem ffs;
#if HYFS_HIDE_FLAGS == 1
	Vector<fileid_t> hiddenFwFiles;
#endif
};

} // namespace HYFS

} // namespace IFS
