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
#include "Object.h"
#ifdef FWFS_OBJECT_CACHE
#include "ObjRefCache.h"
#endif

namespace IFS
{
namespace FWFS
{
// First object located immediately after start marker in image
constexpr size_t FWFS_BASE_OFFSET{sizeof(uint32_t)};

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
 * @brief FWFS Volume definition - identifies filesystem and volume object after mounting
 */
struct FWVolume {
	std::unique_ptr<IFileSystem> fileSystem;
};

/**
 * @brief Implementation of firmware filing system using IFS
 */
class FileSystem : public IFileSystem
{
public:
	FileSystem(Storage::Partition partition) : partition(partition)
	{
	}

	/** @brief Set volume for mountpoint
	 *  @param num Number of mountpoint
	 *  @param fileSystem The filesystem to root at this mountpoint
	 *  @retval int error code
	 */
	int setVolume(uint8_t num, IFileSystem* fileSystem);

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

	int getMd5Hash(FileHandle file, void* buffer, size_t bufSize);

private:
	bool isMounted()
	{
		return flags[Flag::mounted];
	}

	int openRootObject(FWObjDesc& odRoot);

	/**
	 * @brief read a root object header
	 * @param od object descriptor, with offset and ID fields initialised
	 * @retval error code
	 * @note this method deals with top-level objects only
	 */
	int readObjectHeader(FWObjDesc& od)
	{
		//	debug_d("readObject(0x%08X), offset = 0x%08X, sod = %u", &od, od.offset, sizeof(od.obj));
		++od.ref.readCount;
		// First object ID is 1
		if(od.ref.offset == 0) {
			od.ref.id = 1;
		}
		if(!partition.read(FWFS_BASE_OFFSET + od.ref.offset, od.obj)) {
			return Error::ReadFailure;
		}

		/*
			During volume mount, readObjectHeader() is called repeatedly to iterate base objects.
			We had this check for consecutive IDs but it doesn't apply to SPIFFS which allocates
			its own IDs. Also, readObjectHeader might be used on child objects in which case the ID
			isn't applicable. So just leave this code here until I decide what to do with it.

			ID lastObjectID{0};
			...
			// This check is only applicable to FWRO
			if(od.ref.id < lastObjectID) {
				debug_e("FWFS object ID mismatch at 0x%08X: found %u, last ID was %u", od.ref.offset, od.ref.id, lastObjectID);
				res = Error::BadFileSystem;
				break;
			}
			lastObjectID = od.ref.id;
		*/

#ifdef FWFS_OBJECT_CACHE
		if(isMounted()) {
			cache.improve(od.ref, objIndex);
		} else {
			cache.add(od.ref);
		}
#endif

		return FS_OK;
	}

	/**
	 * @brief find an object and return a descriptor for it
	 * @param od IN/OUT: resolved object
	 * @retval int error code
	 * @note od.ref must be initialised
	 */
	int openObject(FWObjDesc& od)
	{
		// The object we're looking for
		Object::ID objId = od.ref.id;

		// Start search with first child
		od.ref.offset = 0;
		od.ref.id = 1; // We don't use id #0

		if(objId > lastFound.id && lastFound.id > od.ref.id) {
			od.ref = lastFound;
		}

		int res;
		while((res = readObjectHeader(od)) >= 0 && od.ref.id != objId) {
			od.next();
		}

		if(res >= 0) {
			lastFound = od.ref;
		} else if(res == Error::NoMoreFiles) {
			res = Error::NotFound;
		}

		return res;
	}

	/**
	 * @brief open a descriptor for a child object
	 * @param parent
	 * @param child reference to child, relative to parent
	 * @param od OUT: resolved object
	 * @retval int error code
	 */
	int openChildObject(const FWObjDesc& parent, const FWObjDesc& child, FWObjDesc& od)
	{
		if(!child.obj.isRef()) {
			od = child;
			od.ref.offset += parent.childTableOffset();
			return FS_OK;
		}

		od.ref = ObjRef(child.obj.data8.ref.id);

		int res = openObject(od);
		if(res < 0) {
			return res;
		}

		if(od.obj.type() != child.obj.type()) {
			// Reference must point to object of same type
			return Error::BadObject;
		}

		if(od.obj.isRef()) {
			// Reference must point to an actual object, not another reference
			return Error::BadObject;
		}

		return FS_OK;
	}

	/**
	 * @brief fetch child object header
	 * @param parent
	 * @param child uninitialised child, returns result
	 * @retval error code
	 * @note references are not pursued; the caller must handle that
	 * child.ref refers to position relative to parent
	 * Implementations must set child.storenum = parent.storenum; other values
	 * will be meaningless as object stores are unaware of other stores.
	 */
	int readChildObjectHeader(const FWObjDesc& parent, FWObjDesc& child)
	{
		assert(parent.obj.isNamed());

		if(child.ref.offset >= parent.obj.childTableSize()) {
			return Error::EndOfObjects;
		}

		// Get the absolute offset for the child object
		uint32_t tableOffset = parent.childTableOffset();
		child.ref.offset += tableOffset;
		int res = readObjectHeader(child);
		child.ref.offset -= tableOffset;
		return res;
	}

	/**
	 * @brief read object content
	 * @param offset location to start reading, from start of object content
	 * @param size bytes to read
	 * @param buffer to store data
	 * @retval number of bytes read, or error code
	 * @note must fail if cannot read all requested bytes
	 */
	int readObjectContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer)
	{
		offset += FWFS_BASE_OFFSET + od.contentOffset();
		return partition.read(offset, buffer, size) ? FS_OK : Error::ReadFailure;
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
	enum class Flag {
		mounted,
	};

	Storage::Partition partition;
	ObjRef volume;
	ObjRef lastFound;
#ifdef FWFS_OBJECT_CACHE
	ObjRefCache cache;
#endif
	BitSet<uint8_t, Flag> flags;
};

} // namespace FWFS

} // namespace IFS
