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

#if FWFS_CACHE_SPACING
#include "ObjRefCache.h"
#endif

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
 * @brief FWFS File Descriptor
 */
struct FWFileDesc {
	FWObjDesc odFile; ///< File object
	union {
		struct {
			uint32_t dataSize; ///< Total size of data
			uint32_t cursor;   ///< Current read/write offset within file data
		};
		// For MountPoint
		struct {
			IFileSystem* fileSystem;
			union {
				FileHandle file;
				DirHandle dir;
			};
		};
	};

	bool isAllocated() const
	{
		return odFile.obj.typeData != 0;
	}

	bool isMountPoint() const
	{
		return odFile.obj.isMountPoint();
	}

	void reset()
	{
		*this = FWFileDesc{};
	}
};

/**
 * @brief FWFS Volume definition for mount points
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

	// IFileSystem methods
	int mount() override;
	int getinfo(Info& info) override;
	int setVolume(uint8_t index, IFileSystem* fileSystem) override;
	int opendir(const char* path, DirHandle& dir) override;
	int readdir(DirHandle dir, Stat& stat) override;
	int rewinddir(DirHandle dir) override;
	int closedir(DirHandle dir) override;
	int mkdir(const char* path) override;
	int stat(const char* path, Stat* stat) override;
	int fstat(FileHandle file, Stat* stat) override;
	int fcontrol(FileHandle file, ControlCode code, void* buffer, size_t bufSize) override;
	int fsetxattr(FileHandle file, AttributeTag tag, const void* data, size_t size) override;
	int fgetxattr(FileHandle file, AttributeTag tag, void* buffer, size_t size) override;
	int setxattr(const char* path, AttributeTag tag, const void* data, size_t size) override;
	int getxattr(const char* path, AttributeTag tag, void* buffer, size_t size) override;
	FileHandle open(const char* path, OpenFlags flags) override;
	int close(FileHandle file) override;
	int read(FileHandle file, void* data, size_t size) override;
	int write(FileHandle file, const void* data, size_t size) override;
	int lseek(FileHandle file, int offset, SeekOrigin origin) override;
	int eof(FileHandle file) override;
	int32_t tell(FileHandle file) override;
	int ftruncate(FileHandle file, size_t new_size) override;
	int flush(FileHandle file) override;
	int rename(const char* oldpath, const char* newpath) override;
	int remove(const char* path) override;
	int fremove(FileHandle file) override;
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

private:
	int getMd5Hash(FWFileDesc& fd, void* buffer, size_t bufSize);

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
			od.ref.offset = FWFS_BASE_OFFSET;
		}
		return partition.read(od.ref.offset, od.obj) ? FS_OK : Error::ReadFailure;
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

		// The object we're looking for
		Object::ID objId = child.obj.data8.ref.id;

		if(objId > lastFound.id) {
			od.ref = lastFound;
		} else {
			od.ref.offset = FWFS_BASE_OFFSET;
			od.ref.id = 1; // We don't use id #0
		}

#if FWFS_CACHE_SPACING
		cache.improve(od.ref, objId);
#endif

		for(;;) {
			int res = readObjectHeader(od);
			if(res < 0) {
				return (res == Error::NoMoreFiles) ? Error::NotFound : res;
			}
			if(od.ref.id > objId) {
				return Error::NotFound;
			}
			if(od.ref.id == objId) {
				break;
			}
			od.next();
		}

		lastFound = od.ref;

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
		offset += od.contentOffset();
		return partition.read(offset, buffer, size) ? FS_OK : Error::ReadFailure;
	}

	/**
	 * @brief Find an unused descriptor
	 * @retval int index of descriptor, or error code
	 */
	int findUnusedDescriptor();

	int findChildObjectHeader(const FWObjDesc& parent, FWObjDesc& child, Object::Type objId);
	int findChildObject(const FWObjDesc& parent, FWObjDesc& child, const char* name, unsigned namelen);

	/**
	 * @brief Parse a file path to locate the corresponding object
	 * @param path Path to parse. If a mountpoint is located this returns remainder of path
	 * @param od Located object
	 * 
	 * Parsing stops if a mountpoint is found.
	 */
	int findObjectByPath(const char*& path, FWObjDesc& od);

	/**
	 * @brief Resolve a mountpoint object to mounted filesystem
	 * @param odMountPoint The mountpoint object to resolve
	 * @param fileSystem OUT: the corresponding mounted filesystem
	 * @retval int error code
	 */
	int resolveMountPoint(const FWObjDesc& odMountPoint, IFileSystem*& fileSystem);

	/*
	 * @brief Resolve path to mounted volume.
	 * @param path Path to parse, consumes path to mount point
	 * @param fileSystem Located filesystem
	 * @retval int error code
	 * 
	 * Used for methods which require write access are read-only unless path corresponds to mounted volume.
	 * If path is within this volume then Error::ReadOnly is returned.
	 */
	int findLinkedObject(const char*& path, IFileSystem*& fileSystem);

	/**
	 * @brief Get total size for all contained data objects
	 * @param od A named object
	 * @param dataSize The total size in bytes
	 * @retval int error code
	 */
	int getObjectDataSize(FWObjDesc& od, size_t& dataSize);

	int readObjectName(const FWObjDesc& od, NameBuffer& name);
	int fillStat(Stat& stat, const FWObjDesc& entry);
	int readAttribute(Stat& stat, AttributeTag tag, void* buffer, size_t size);

	void printObject(const FWObjDesc& od);

private:
	FWVolume volumes[FWFS_MAX_VOLUMES]; ///< Volumes mapped to mountpoints by index
	ACL rootACL;
	FWFileDesc fileDescriptors[FWFS_MAX_FDS];
	enum class Flag {
		mounted,
	};

	Storage::Partition partition;
	ObjRef volume;	///< Reference to main volume object
	ObjRef lastFound; ///< Speeds up consective searches
#if FWFS_CACHE_SPACING
	ObjRefCache cache;
#endif
	BitSet<uint8_t, Flag> flags;
};

} // namespace FWFS

} // namespace IFS
