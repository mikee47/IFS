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

#include "FirmwareFileSystem.h"
#include "SPIFlashFileSystem.h"
#include "ifstypes.h"

#ifndef HYFS_HIDE_FLAGS
#define HYFS_HIDE_FLAGS 1
#endif

#if HYFS_HIDE_FLAGS == 1
#include "WVector.h"
#endif

class HybridFileSystem : public IFileSystem
{
public:
	HybridFileSystem(IFSObjectStore* fwStore, IFSMedia* ffsMedia) : _fw(fwStore), _ffs(ffsMedia)
	{
	}

	~HybridFileSystem() override
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
	int isfile(file_t file) override
	{
		int res = _fw.isfile(file);
		if(res != FS_OK)
			res = _ffs.isfile(file);
		return res;
	}

private:
	int _hideFWFile(const char* path, bool hide);
	bool _isFWFileHidden(const FileStat& fwstat);

private:
	FirmwareFileSystem _fw;
	SPIFlashFileSystem _ffs;
#if HYFS_HIDE_FLAGS == 1
	Vector<fileid_t> m_hiddenFWFiles;
#endif
};
