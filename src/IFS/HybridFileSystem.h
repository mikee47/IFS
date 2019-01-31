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

#ifndef __HYBRID_FILESYSTEM_H
#define __HYBRID_FILESYSTEM_H

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

	virtual ~HybridFileSystem()
	{
	}

	// IFileSystem methods
	virtual int mount();
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
	virtual int isfile(file_t file)
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

#endif // __HYBRID_FILESYSTEM_H
