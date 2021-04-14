/**
 * FileSystem.h
 * FWFS - Firmware File System
 *
 * Created on: 19 Jul 2018
 *
 * Copyright 2019 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the IFS Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "../FileSystem.h"
#include "../ObjectStore.h"

namespace IFS
{
namespace FWFS
{
// File handles start at this value
#ifndef FWFS_HANDLE_MIN
#define FWFS_HANDLE_MIN 100
#endif

// Maximum number of file descriptors
#ifndef FWFS_MAX_FDS
#define FWFS_MAX_FDS 8
#endif

// Maximum number of volumes - 1 is minimum, the rest are mounted in subdirectories
#ifndef FWFS_MAX_VOLUMES
#define FWFS_MAX_VOLUMES 4
#endif

// Maximum file handle value
#define FWFS_HANDLE_MAX (FWFS_HANDLE_MIN + FWFS_MAX_FDS - 1)

/**
 * @brief file descriptor attributes
 * @note these are bit values, combine using _BV()
 */
enum class FWFileDescAttr {
	allocated, ///< Descriptor in use
};

using FWFileDescAttributes = BitSet<uint8_t, FWFileDescAttr, 1>;

/**
 * @brief FWFS File Descriptor
 */
struct FWFileDesc {
	FWObjDesc odFile;	 ///< File object
	uint32_t dataSize{0}; ///< Total size of data
	uint32_t cursor{0};   ///< Current read/write offset within file data
	FWFileDescAttributes attr;
};

/**
 * @brief FWFS Volume definition - identifies object store and volume object after mounting
 */
struct FWVolume {
	IObjectStore* store{nullptr};
	ObjRef ref; ///< Volume reference

	bool isMounted()
	{
		return store ? store->isMounted() : false;
	}
};

/**
 * @brief Implementation of firmware filing system using IFS
 */
class FileSystem : public IFileSystem
{
public:
	FileSystem()
	{
	}

	FileSystem(IObjectStore* store)
	{
		setVolume(0, store);
	}

	~FileSystem()
	{
		for(auto& vol : volumes) {
			delete vol.store;
		}
	}

	/** @brief Set object stores
	 *  @param num which store to set
	 *  @param store the object store object
	 *  @retval int error code
	 */
	int setVolume(uint8_t num, IObjectStore* store);

	// IFileSystem methods
	int mount() override;
	int getinfo(Info& info) override;
	int opendir(const char* path, DirHandle& dir) override;
	int readdir(DirHandle dir, Stat& stat) override;
	int rewinddir(DirHandle dir) override;
	int closedir(DirHandle dir) override;
	int mkdir(const char* path) override
	{
		return Error::ReadOnly;
	}
	int mkdirat(DirHandle dir, const char* name) override
	{
		return Error::ReadOnly;
	}
	int stat(const char* path, Stat* stat) override;
	int fstat(FileHandle file, Stat* stat) override;
	int fcontrol(FileHandle file, ControlCode code, void* buffer, size_t bufSize) override;
	int fsetxattr(FileHandle file, AttributeTag tag, const void* data, size_t size) override
	{
		return Error::ReadOnly;
	}
	int fgetxattr(FileHandle file, AttributeTag tag, void* buffer, size_t size) override;
	int setxattr(const char* path, AttributeTag tag, const void* data, size_t size) override
	{
		return Error::ReadOnly;
	}
	int getxattr(const char* path, AttributeTag tag, void* buffer, size_t size) override;
	FileHandle open(const char* path, OpenFlags flags) override;
	FileHandle openat(DirHandle dir, const char* name, OpenFlags flags) override;
	int close(FileHandle file) override;
	int read(FileHandle file, void* data, size_t size) override;
	int write(FileHandle file, const void* data, size_t size) override
	{
		return Error::ReadOnly;
	}
	int lseek(FileHandle file, int offset, SeekOrigin origin) override;
	int eof(FileHandle file) override;
	int32_t tell(FileHandle file) override;
	int ftruncate(FileHandle file, size_t new_size) override
	{
		return Error::ReadOnly;
	}
	int flush(FileHandle file) override
	{
		return Error::ReadOnly;
	}
	int rename(const char* oldpath, const char* newpath) override
	{
		return Error::ReadOnly;
	}
	int remove(const char* path) override
	{
		return Error::ReadOnly;
	}
	int fremove(FileHandle file) override
	{
		return Error::ReadOnly;
	}
	int format() override
	{
		return Error::ReadOnly;
	}
	int check() override
	{
		/* We could implement this, but since problems would indicate corrupted firmware
    	 * there isn't much we can do other than suggest a re-flashing. This sort of issue
    	 * is better resolved externally using a hash of the entire firmware image. */
		return Error::NotImplemented;
	}

	/** @brief get the full path of a file from its ID
	 *  @param fileid
	 *  @param path
	 *  @retval int error code
	 */
	int getFilePath(FileID fileid, NameBuffer& path);

	int getMd5Hash(FileHandle file, void* buffer, size_t bufSize);

private:
	int seekFilePath(FWObjDesc& parent, FileID fileid, NameBuffer& path);

	/** @brief Mount the given volume, scanning its contents for verification
	 *  @param volume IN: identifies object store, OUT: contains volume object reference
	 *  @retval int error code
	 */
	int mountVolume(FWVolume& volume);

	/** @brief Obtain the managing object store for an object reference
	 *  @param ref .store field must be valid */
	int getStore(const ObjRef& ref, IObjectStore*& store)
	{
		if(ref.storenum >= FWFS_MAX_VOLUMES) {
			return Error::BadStore;
		}
		store = volumes[ref.storenum].store;
		return store ? FS_OK : Error::NotMounted;
	}

	int getStore(const FWObjDesc& od, IObjectStore*& store)
	{
		return getStore(od.ref, store);
	}

	int openRootObject(const FWVolume& volume, FWObjDesc& odRoot);

	int openObject(FWObjDesc& od)
	{
		assert(od.ref.refCount == 0);

		IObjectStore* store;
		int res = getStore(od.ref, store);
		if(res >= 0) {
			res = store->open(od);
			if(res >= 0) {
				++od.ref.refCount;
			}
		}

		return res;
	}

	int openChildObject(const FWObjDesc& parent, const FWObjDesc& child, FWObjDesc& od)
	{
		assert(od.ref.refCount == 0);

		IObjectStore* store;
		int res = getStore(parent.ref, store);
		if(res >= 0) {
			res = store->openChild(parent, child, od);
			if(res >= 0) {
				++od.ref.refCount;
			}
		}

		return res;
	}

	int closeObject(FWObjDesc& od)
	{
		assert(od.ref.refCount == 1);

		IObjectStore* store;
		int res = getStore(od.ref, store);
		if(res >= 0) {
			res = store->close(od);
			if(res >= 0) {
				--od.ref.refCount;
			}
		}

		return res;
	}

	int readObjectHeader(FWObjDesc& od)
	{
		IObjectStore* store;
		int res = getStore(od.ref, store);
		return res < 0 ? res : store->readHeader(od);
	}

	int readChildObjectHeader(const FWObjDesc& parent, FWObjDesc& child)
	{
		assert(parent.obj.isNamed());
		// Child must be in same store as parent
		child.ref.storenum = parent.ref.storenum;
		IObjectStore* store;
		int res = getStore(parent.ref, store);
		return res < 0 ? res : store->readChildHeader(parent, child);
	}

	int readObjectContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer)
	{
		IObjectStore* store;
		int res = getStore(od.ref, store);
		return res < 0 ? res : store->readContent(od, offset, size, buffer);
	}

	/** @brief Find an unused descriptor
	 *  @retval int index of descriptor, or error code
	 */
	int findUnusedDescriptor();

	int findChildObjectHeader(const FWObjDesc& parent, FWObjDesc& child, Object::Type objId);
	int findChildObject(const FWObjDesc& parent, FWObjDesc& child, const char* name, unsigned namelen);
	int findObjectByPath(const char* path, FWObjDesc& od);
	int resolveMountPoint(const FWObjDesc& odMountPoint, FWObjDesc& odResolved);

	int readObjectName(const FWObjDesc& od, NameBuffer& name);
	FileHandle allocateFileDescriptor(FWObjDesc& odFile);
	int fillStat(Stat& stat, const FWObjDesc& entry);
	int readAttribute(Stat& stat, AttributeTag tag, void* buffer, size_t size);

	void printObject(const FWObjDesc& od);

private:
	FWVolume volumes[FWFS_MAX_VOLUMES]; ///< Store 0 contains the root filing system
	ACL rootACL;
	FWFileDesc fileDescriptors[FWFS_MAX_FDS];
};

} // namespace FWFS

} // namespace IFS
