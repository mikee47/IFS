/*
 * FirmwareFileSystem.h
 *
 *  Created on: 19 Jul 2018
 *      Author: mikee47
 *
 * FWFS - Firmware File System
 *
 * A read-only layout which uses an image baked into the firmware.
 * It uses very little RAM and supports all IFS features, except those which
 * modify the layout.
 *
 * Layouts are built using a simple python script, so easily modified to add features.
 * It supports access control, compression, timestamps and file attributes as well
 * as multiple directories.
 *
 *
 *
 */

#ifndef __FIRMWARE_FILESYSTEM_H
#define __FIRMWARE_FILESYSTEM_H

#include "IFS.h"
#include "IFSObjectStore.h"

// File handles start at this value
#ifndef FWFILE_HANDLE_MIN
#define FWFS_HANDLE_MIN 100
#endif

// Maximum number of file descriptors
#ifndef FWFILE_MAX_FDS
#define FWFS_MAX_FDS 8
#endif

// Maximum number of volumes - 1 is minimum, the rest are mounted in subdirectories
#ifndef FWFS_MAX_VOLUMES
#define FWFS_MAX_VOLUMES 4
#endif

// Maximum file handle value
#define FWFS_HANDLE_MAX (FWFS_HANDLE_MIN + FWFS_MAX_FDS - 1)

/** @brief file descriptor attributes
 *  @note these are bit values, combine using _BV()
 */
enum FWFileDescAttr {
	fwfda_Allocated, ///< Descriptor in use
};

typedef uint8_t FWFileDescAttributes;

/** @brief FWFS File Descriptor
 */
struct FWFileDesc {
	FWObjDesc odFile;	  ///< File object
	uint32_t dataSize = 0; ///< Total size of data
	uint32_t cursor = 0;   ///< Current read/write offset within file data
	FWFileDescAttributes attr = 0;
};

/** @brief FWFS Volume definition - identifies object store and volume object after mounting
 */
struct FWVolume {
	IFSObjectStore* store = nullptr;
	FWObjRef ref; ///< Volume reference

	bool isMounted()
	{
		return store ? store->isMounted() : false;
	}
};

/** @brief Implementation of firmware filing system using IFS
 */
class FirmwareFileSystem : public IFileSystem
{
public:
	FirmwareFileSystem()
	{
	}

	FirmwareFileSystem(IFSObjectStore* store)
	{
		setVolume(0, store);
	}

	virtual ~FirmwareFileSystem()
	{
		for(auto& vol : _volumes)
			delete vol.store;
	}

	/** @brief Set object stores
	 *  @param num which store to set
	 *  @param store the object store object
	 *  @retval int error code
	 */
	int setVolume(uint8_t num, IFSObjectStore* store);

	int mount();

	// IFileSystem methods
	virtual int getinfo(FileSystemInfo& info);
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
	int seekFilePath(FWObjDesc& parent, fileid_t fileid, NameBuffer& path);

	/** @brief Mount the given volume, scanning its contents for verification
	 *  @param volume IN: identifies object store, OUT: contains volume object reference
	 *  @retval int error code
	 */
	int mountVolume(FWVolume& volume);

	/** @brief Obtain the managing object store for an object reference
	 *  @param ref .store field must be valid */
	int getStore(const FWObjRef& ref, IFSObjectStore*& store)
	{
		if(ref.storenum >= FWFS_MAX_VOLUMES)
			return FSERR_BadStore;
		store = _volumes[ref.storenum].store;
		return store ? FS_OK : FSERR_NotMounted;
	}

	int getStore(const FWObjDesc& od, IFSObjectStore*& store)
	{
		return getStore(od.ref, store);
	}

	int openRootObject(const FWVolume& volume, FWObjDesc& odRoot);

	int openObject(FWObjDesc& od)
	{
		assert(od.ref.refCount == 0);

		IFSObjectStore* store;
		int res = getStore(od.ref, store);
		if(res >= 0) {
			res = store->open(od);
			if(res >= 0)
				++od.ref.refCount;
		}

		return res;
	}

	int openChildObject(const FWObjDesc& parent, const FWObjDesc& child, FWObjDesc& od)
	{
		assert(od.ref.refCount == 0);

		IFSObjectStore* store;
		int res = getStore(parent.ref, store);
		if(res >= 0) {
			res = store->openChild(parent, child, od);
			if(res >= 0)
				++od.ref.refCount;
		}

		return res;
	}

	int closeObject(FWObjDesc& od)
	{
		assert(od.ref.refCount == 1);

		IFSObjectStore* store;
		int res = getStore(od.ref, store);
		if(res >= 0) {
			res = store->close(od);
			if(res >= 0)
				--od.ref.refCount;
		}

		return res;
	}

	int readObjectHeader(FWObjDesc& od)
	{
		IFSObjectStore* store;
		int res = getStore(od.ref, store);
		return res < 0 ? res : store->readHeader(od);
	}

	int readChildObjectHeader(const FWObjDesc& parent, FWObjDesc& child)
	{
		assert(parent.obj.isNamed());
		// Child must be in same store as parent
		child.ref.storenum = parent.ref.storenum;
		IFSObjectStore* store;
		int res = getStore(parent.ref, store);
		return res < 0 ? res : store->readChildHeader(parent, child);
	}

	int readObjectContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer)
	{
		IFSObjectStore* store;
		int res = getStore(od.ref, store);
		return res < 0 ? res : store->readContent(od, offset, size, buffer);
	}

	/** @brief Find an unused descriptor
	 *  @retval int index of descriptor, or error code
	 */
	int findUnusedDescriptor();

	int findChildObjectHeader(const FWObjDesc& parent, FWObjDesc& child, FWFS_ObjectType objId);
	int findChildObject(const FWObjDesc& parent, FWObjDesc& child, const char* name, unsigned namelen);
	int findObjectByPath(const char* path, FWObjDesc& od);
	int resolveMountPoint(const FWObjDesc& odMountPoint, FWObjDesc& odResolved);

	int readObjectName(const FWObjDesc& od, NameBuffer& name);
	file_t allocateFileDescriptor(FWObjDesc& odFile);
	int fillStat(FileStat& stat, const FWObjDesc& entry);

	void printObject(const FWObjDesc& od);

private:
	FWVolume _volumes[FWFS_MAX_VOLUMES]; ///< Store 0 contains the root filing system
	FileACL _rootACL;
	FWFileDesc _fds[FWFS_MAX_FDS]; ///< File descriptor array
};

#endif // __FIRMWARE_FILESYSTEM_H
