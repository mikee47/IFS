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
	int getinfo(Info& info) override;
	String getErrorString(int err) override;
	int opendir(const char* path, DirHandle& dir) override;
	int readdir(DirHandle dir, FileStat& stat) override;
	int rewinddir(DirHandle dir) override;
	int closedir(DirHandle dir) override;
	int mkdir(const char* path) override;
	int stat(const char* path, FileStat* stat) override;
	int fstat(File::Handle file, FileStat* stat) override;
	int setacl(File::Handle file, const File::ACL& acl) override;
	int setattr(File::Handle file, File::Attributes attr) override;
	int settime(File::Handle file, time_t mtime) override;
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
	int isfile(File::Handle file) override
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
	Vector<File::ID> hiddenFwFiles;
#endif
};

} // namespace HYFS

} // namespace IFS
