/**
 * FileSystem.cpp
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
 ****/

#include "../include/IFS/FWFS/FileSystem.h"
#include "../include/IFS/Object.h"
#include "../include/IFS/Util.h"

namespace IFS
{
namespace FWFS
{
/*
 * Macros to perform standard checks
 */

#define CHECK_MOUNTED()                                                                                                \
	if(!isMounted()) {                                                                                                 \
		return Error::NotMounted;                                                                                      \
	}

#define GET_FD()                                                                                                       \
	CHECK_MOUNTED()                                                                                                    \
	if(file < FWFS_HANDLE_MIN || file > FWFS_HANDLE_MAX) {                                                             \
		return Error::InvalidHandle;                                                                                   \
	}                                                                                                                  \
	auto& fd = fileDescriptors[file - FWFS_HANDLE_MIN];                                                                \
	if(!fd.attr[FWFileDescAttr::allocated]) {                                                                          \
		return Error::FileNotOpen;                                                                                     \
	}

void FileSystem::printObject(const FWObjDesc& od)
{
	char name[260];
	if(od.obj.isNamed()) {
		name[0] = ' ';
		name[1] = '"';
		unsigned maxlen = sizeof(name) - 4;
		auto& objNamed = od.obj.data16.named;
		unsigned namelen = objNamed.namelen;
		readObjectContent(od, objNamed.nameOffset(), namelen, &name[2]);
		if(namelen > maxlen) {
			namelen = maxlen;
			name[2 + namelen - 1] = '?';
		}
		name[2 + namelen] = '"';
		name[3 + namelen] = '\0';
	} else {
		name[0] = '\0';
	}
	debug_d("@0x%08X #%u: id = 0x%02X, %u bytes - %s%s", od.ref.offset, od.ref.id, od.obj.typeData, od.obj.size(),
			toString(od.obj.type()).c_str(), name);
	//	if (od.obj.size() <= 4)
	//		debug_hex(INFO, "OBJ", &od.obj, od.obj.size());
}

/** @brief initialise Stat structure and copy information from directory entry
 *  @param stat
 *  @param entry
 */
int FileSystem::fillStat(Stat& stat, const FWObjDesc& entry)
{
	assert(entry.obj.isNamed());

	stat = Stat{};
	stat.fs = this;
	stat.id = entry.ref.id;
	stat.mtime = entry.obj.data16.named.mtime;
	stat.acl = rootACL;

	int res;
	FWObjDesc child;
	while((res = readChildObjectHeader(entry, child)) >= 0) {
		if(child.obj.isNamed()) {
			child.next();
			continue;
		}

		if(child.obj.isData()) {
			if(child.obj.isRef()) {
				FWObjDesc od;
				res = openChildObject(entry, child, od);
				if(res < 0) {
					return res;
				}
				stat.size += od.obj.contentSize();
			} else {
				stat.size += child.obj.contentSize();
			}
		} else {
			switch(child.obj.type()) {
			case Object::Type::ObjAttr: {
				Object::Attributes attr = child.obj.data8.objectAttributes.attr;
				if(attr[Object::Attribute::ReadOnly]) {
					stat.attr |= FileAttribute::ReadOnly;
				}
				if(attr[Object::Attribute::Archive]) {
					stat.attr |= FileAttribute::Archive;
				}
				break;
			}

			case Object::Type::ObjectStore:
				stat.attr |= FileAttribute::MountPoint + FileAttribute::Directory;
				break;

			case Object::Type::Compression:
				stat.attr |= FileAttribute::Compressed;
				stat.compression.type = child.obj.data8.compression.type;
				stat.compression.originalSize = child.obj.data8.compression.originalSize;
				break;

			case Object::Type::ReadACE:
				stat.acl.readAccess = child.obj.data8.ace.role;
				break;

			case Object::Type::WriteACE:
				stat.acl.writeAccess = child.obj.data8.ace.role;
				break;

			default:; // Not interested
			}
		}

		child.next();
	}

	if(entry.obj.type() == Object::Type::Directory) {
		stat.attr |= FileAttribute::Directory;
	}

	if(!stat.attr[FileAttribute::Compressed]) {
		stat.compression.originalSize = stat.size;
	}

	return readObjectName(entry, stat.name);
}

int FileSystem::findUnusedDescriptor()
{
	for(int i = 0; i < FWFS_MAX_FDS; ++i) {
		if(!fileDescriptors[i].attr[FWFileDescAttr::allocated]) {
			return i;
		}
	}

	return Error::OutOfFileDescs;
}

int FileSystem::read(FileHandle file, void* data, size_t size)
{
	GET_FD();

	uint32_t readTotal = 0;
	// Offset from start of data content
	struct {
		uint32_t start;
		size_t length;
	} ext{};

	FWObjDesc child;
	int res;
	while((res = readChildObjectHeader(fd.odFile, child)) >= 0) {
		if(child.obj.isData()) {
			FWObjDesc odData;
			res = openChildObject(fd.odFile, child, odData);
			if(res < 0) {
				return res;
			}

			ext.length = odData.obj.contentSize();
			// Do we need data from this object ?
			if(fd.cursor >= ext.start) {
				auto offset = fd.cursor - ext.start;
				auto readlen = std::min(ext.length - offset, size - readTotal);
				res = readObjectContent(odData, fd.cursor - ext.start, readlen, at_offset<void*>(data, readTotal));
				if(res >= 0) {
					fd.cursor += readlen;
					readTotal += readlen;
				}
			}

			ext.start += ext.length;

			if(res < 0 || readTotal == size) {
				break;
			}
		}

		child.next();
	}

	return (res == FS_OK) || (res == Error::EndOfObjects) ? readTotal : res;
}

int FileSystem::lseek(FileHandle file, int offset, SeekOrigin origin)
{
	GET_FD();

	int newOffset = offset;
	if(origin == SeekOrigin::Current) {
		newOffset += (int)fd.cursor;
	} else if(origin == SeekOrigin::End) {
		newOffset += (int)fd.dataSize;
	}

	//	debug_d("lseek(%d, %d, %d): %d", file, offset, origin, newOffset);

	if((uint32_t)newOffset > fd.dataSize) {
		return Error::SeekBounds;
	}

	fd.cursor = newOffset;
	return newOffset;
}

int FileSystem::setVolume(uint8_t num, IFileSystem* fileSystem)
{
	if(num >= FWFS_MAX_VOLUMES || fileSystem == this) {
		return Error::BadVolume;
	}

	volumes[num].fileSystem.reset(fileSystem);
	return FS_OK;
}

int FileSystem::mount()
{
	if(!partition) {
		return Error::NoPartition;
	}

	if(!partition.verify(Storage::Partition::SubType::Data::fwfs)) {
		return Error::BadPartition;
	}

	uint32_t marker;
	if(!partition.read(0, marker)) {
		return Error::ReadFailure;
	}

	if(marker != FWFILESYS_START_MARKER) {
		debug_e("Filesys start marker invalid: found 0x%08x, expected 0x%08x", marker, FWFILESYS_START_MARKER);
		return Error::BadFileSystem;
	}

	unsigned objectCount = 0;
	FWObjDesc od;
	int res;
	while((res = readObjectHeader(od)) >= 0) {
		++objectCount;
		//		printObject(od);

		if(od.obj.type() == Object::Type::Volume) {
			volume = od.ref;
		} else if(od.obj.type() == Object::Type::End) {
			// @todo verify checksum
			// od.obj.end.checksum
			break;
		}

		od.next();
	}

	debug_d("Ended @ 0x%08X (#%u), %u objects, volume @ 0x%08X, od @ 0x%08X", od.ref.offset, od.ref.offset, od.ref.id,
			objectCount, volume.offset);

	if(res < 0) {
		return res;
	}

	if(volume.offset == 0) {
		debug_e("Volume object missing");
		return Error::BadFileSystem;
	}

	// Having scanned all the objects, check the end marker
	auto offset = FWFS_BASE_OFFSET + od.nextOffset();
	if(partition.read(offset, marker)) {
		if(marker != FWFILESYS_END_MARKER) {
			debug_e("Filesys end marker invalid: found 0x%08x, expected 0x%08x", marker, FWFILESYS_END_MARKER);
			return Error::BadFileSystem;
		}
	}

#ifdef FWFS_OBJECT_CACHE
	cache.initialise(od.obj.id + 1);
#endif

	FWObjDesc odRoot;
	res = openRootObject(odRoot);
	if(res < 0) {
		return res;
	}
	Stat stat;
	fillStat(stat, odRoot);
	rootACL = stat.acl;

	flags[Flag::mounted] = true;

	return FS_OK;
}

int FileSystem::openRootObject(FWObjDesc& odRoot)
{
	FWObjDesc odVolume(volume);
	int res = readObjectHeader(odVolume);
	if(res >= 0) {
		FWObjDesc child;
		res = findChildObjectHeader(odVolume, child, Object::Type::Directory);
		if(res >= 0) {
			res = openChildObject(odVolume, child, odRoot);
		}
	}

	if(res < 0) {
		debug_e("Problem reading root directory %d", res);
	}

	return res;
}

int FileSystem::getinfo(Info& info)
{
	int res = FS_OK;

	info.clear();
	info.type = Type::FWFS;
	info.maxNameLength = INT16_MAX;
	info.maxPathLength = INT16_MAX;
	info.attr = Attribute::ReadOnly;
	info.partition = partition;
	info.volumeSize = partition.size();

	if(isMounted()) {
		FWObjDesc odVolume(volume);
		res = readObjectHeader(odVolume);
		if(res >= 0) {
			readObjectName(odVolume, info.name);
			FWObjDesc od;
			if(findChildObjectHeader(odVolume, od, Object::Type::ID32)) {
				info.volumeID = od.obj.data8.id32.value;
			}
		}
		info.attr |= Attribute::Mounted;
	}

	return res;
}

int FileSystem::findChildObjectHeader(const FWObjDesc& parent, FWObjDesc& child, Object::Type objId)
{
	int res;
	FWObjDesc od;
	while((res = readChildObjectHeader(parent, od)) >= 0) {
		if(od.obj.type() == objId) {
			child = od;
			break;
		}
		od.next();
	}

	return res == Error::EndOfObjects ? Error::NotFound : res;
}

// int FileSystem::resolveMountPoint(const FWObjDesc& odMountPoint, FWObjDesc& odResolved)
// {
// 	assert(odMountPoint.obj.isMountPoint());

// 	FWObjDesc odStore;
// 	int res = findChildObjectHeader(odMountPoint, odStore, Object::Type::ObjectStore);
// 	if(res < 0) {
// 		debug_e("Mount point missing object store");
// 		return res;
// 	}

// 	auto storenum = odStore.obj.data8.objectStore.storenum;
// 	auto& vol = volumes[storenum];
// 	if(vol.fileSystem == nullptr) {
// 		return Error::NotMounted;
// 	}

// 	// Locate the root directory
// 	return openRootObject(vol, odResolved);
// }

/** @brief find object by name
 *  @param parent container object
 *  @param child returns the located child object  - references are resolved
 *  @param name of object, nullptr is interpreted as empty string
 *  @param namelen length of name - which doesn't have to be nul terminated
 *  @retval error code
 *  @note child and parent must refer to different objects
 */
int FileSystem::findChildObject(const FWObjDesc& parent, FWObjDesc& child, const char* name, unsigned namelen)
{
	assert(parent.obj.isNamed());

	char buf[ALIGNUP4(namelen)];
	int res;
	FWObjDesc od;
	while((res = readChildObjectHeader(parent, od)) >= 0) {
		if(od.obj.isNamed()) {
			res = openChildObject(parent, od, child);
			if(res < 0) {
				break;
			}

			auto& objNamed = child.obj.data16.named;
			if(objNamed.namelen == namelen) {
				if(namelen == 0) {
					break;
				}
				res = readObjectContent(child, objNamed.nameOffset(), ALIGNUP4(namelen), buf);
				if(res < 0) {
					break;
				}

				if(memcmp(buf, name, namelen) == 0) {
					// Matched name
					break;
				}
			}
		}

		od.next();
	}

	return res == Error::EndOfObjects ? Error::NotFound : res;
}

/** @brief read an object name into a buffer, which might be smaller than required.
 *  @param od
 *  @param buffer
 *  @param bufSize
 *  @retval int error code
 *  @note the name is always nul-terminated
 */
int FileSystem::readObjectName(const FWObjDesc& od, NameBuffer& name)
{
	if(name.buffer == nullptr || name.size == 0) {
		return 0;
	}

	assert(od.obj.isNamed());

	auto& objNamed = od.obj.data16.named;
	name.length = objNamed.namelen;
	int res = readObjectContent(od, objNamed.nameOffset(), std::min(name.size, name.length), name.buffer);
	name.terminate();
	return res;
}

/** @brief We have a file object, now get the other details to complete the descriptor
 *  @param od the file object descriptor
 *  @retval FileHandle file handle, or error code
 */
FileHandle FileSystem::allocateFileDescriptor(FWObjDesc& odFile)
{
	int descriptorIndex = findUnusedDescriptor();
	if(descriptorIndex < 0) {
		return descriptorIndex;
	}

	auto& fd = fileDescriptors[descriptorIndex];

	// Default to empty file (i.e. no data object)
	fd.odFile = odFile;
	fd.dataSize = 0;
	fd.cursor = 0;

	int res;
	FWObjDesc child;
	while((res = readChildObjectHeader(fd.odFile, child)) >= 0) {
		if(child.obj.isData()) {
			FWObjDesc odData;
			res = openChildObject(fd.odFile, child, odData);
			if(res < 0) {
				return res;
			}

			fd.dataSize += odData.obj.contentSize();
		}

		child.next();
	}

	debug_d("Descriptor #%u allocated", descriptorIndex);

	fd.attr[FWFileDescAttr::allocated] = true;
	return FWFS_HANDLE_MIN + descriptorIndex;
}

int FileSystem::opendir(const char* path, DirHandle& dir)
{
	CHECK_MOUNTED();

	FWObjDesc od;
	int res = findObjectByPath(path, od);
	if(res >= 0) {
		FileHandle handle = allocateFileDescriptor(od);
		if(handle < 0) {
			res = handle;
		} else {
			dir = reinterpret_cast<DirHandle>(handle);
		}
	}

	return res;
}

/* Reading a directory simply gets details about any child named objects.
 * The XXXdir() methods will work on any named objects, including files, although
 * these don't normally contain named children.
 */
int FileSystem::readdir(DirHandle dir, Stat& stat)
{
	CHECK_MOUNTED();

	auto file = reinterpret_cast<int>(dir);
	GET_FD();

	FWObjDesc odDir = fd.odFile;
	odDir = fd.odFile;

	ObjRef ref;
	ref.offset = fd.cursor;
	FWObjDesc od(ref);
	int res;
	while((res = readChildObjectHeader(odDir, od)) >= 0) {
		if(od.obj.isNamed()) {
			FWObjDesc child;
			res = openChildObject(odDir, od, child);
			if(res >= 0) {
				res = fillStat(stat, child);
			}
			od.next();
			break;
		}

		od.next();
	}

	fd.cursor = od.ref.offset;

	//	debug_d("readdir(), res = %d, od.seekCount = %u", res, od.ref.readCount);

	return res == Error::EndOfObjects ? Error::NoMoreFiles : res;
}

int FileSystem::rewinddir(DirHandle dir)
{
	CHECK_MOUNTED();

	auto file = reinterpret_cast<int>(dir);
	GET_FD();

	fd.cursor = 0;
	return 0;
}

int FileSystem::closedir(DirHandle dir)
{
	return close(reinterpret_cast<int>(dir));
}

/** @brief find object by path
 *  @param path full path for object
 *  @param od descriptor for located object
 *  @retval error code
 *  @note specifying an empty string or nullptr for path will fetch the root directory object
 *  Called by open, opendir and stat methods.
 *  @todo track ACEs during path traversal and store resultant ACL in file descriptor.
 */
int FileSystem::findObjectByPath(const char* path, FWObjDesc& od)
{
#ifdef DEBUG_FWFS
	ElapseTimer timer;
#endif

	// Start with the root directory object
	int res = openRootObject(od);
	if(res < 0) {
		return res;
	}

	// Empty paths indicate root directory
	FS_CHECK_PATH(path);
	if(path == nullptr) {
		return FS_OK;
	}

	const char* tail = path;
	const char* sep;
	do {
		sep = strchr(tail, '/');
		size_t namelen = (sep != nullptr) ? sep - tail : strlen(tail);

		FWObjDesc parent = od;
		res = findChildObject(parent, od, tail, namelen);

		if(res < 0) {
			break;
		}

		tail += namelen + 1;
	} while(sep);

#ifdef DEBUG_FWFS
	debug_d(_F("findObjectByPath('%s'), res = %d, od.seekCount = %u, time = %u us\n"), path, res, od.ref.readCount,
			timer.elapsed());
#endif

	return res;
}

FileHandle FileSystem::open(const char* path, OpenFlags flags)
{
	CHECK_MOUNTED();

	debug_d("open('%s', 0x%02X)", path, flags);

	FWObjDesc od;
	int res = findObjectByPath(path, od);
	if(res >= 0) {
		res = allocateFileDescriptor(od);
	}

	return res;
}

int FileSystem::close(FileHandle file)
{
	GET_FD();

	debug_d("descriptor #%u close", file - FWFS_HANDLE_MIN);

	fd.attr = 0;
	return FS_OK;
}

int FileSystem::stat(const char* path, Stat* stat)
{
	CHECK_MOUNTED();

	FWObjDesc od;
	int res = findObjectByPath(path, od);
	if(res >= 0) {
		if(stat) {
			res = fillStat(*stat, od);
		}
	}
	return res;
}

int FileSystem::fstat(FileHandle file, Stat* stat)
{
	GET_FD();

	return stat ? fillStat(*stat, fd.odFile) : Error::BadParam;
}

int FileSystem::fcontrol(FileHandle file, ControlCode code, void* buffer, size_t bufSize)
{
	switch(code) {
	case FCNTL_GET_MD5_HASH:
		return getMd5Hash(file, buffer, bufSize);
	default:
		return Error::NotSupported;
	}
}

int FileSystem::eof(FileHandle file)
{
	GET_FD();

	// 0 - not EOF, > 0 - at EOF, < 0 - error
	return fd.cursor >= fd.dataSize ? 1 : 0;
}

int32_t FileSystem::tell(FileHandle file)
{
	GET_FD();

	return fd.cursor;
}

int FileSystem::getMd5Hash(FileHandle file, void* buffer, size_t bufSize)
{
	constexpr size_t md5HashSize{16};
	if(bufSize < md5HashSize) {
		return Error::BadParam;
	}

	GET_FD();

	FWObjDesc child;
	int res = findChildObjectHeader(fd.odFile, child, Object::Type::Md5Hash);
	if(res < 0) {
		return res;
	}

	if(child.obj.contentSize() != md5HashSize) {
		return Error::BadObject;
	}
	FWObjDesc od;
	openChildObject(fd.odFile, child, od);
	readObjectContent(od, 0, md5HashSize, buffer);

	return md5HashSize;
}

int FileSystem::readAttribute(Stat& stat, AttributeTag tag, void* buffer, size_t size)
{
	switch(tag) {
	case AttributeTag::ModifiedTime: {
		auto res = sizeof(stat.mtime);
		if(size >= res) {
			memcpy(buffer, &stat.mtime, res);
		}
		return res;
	}

	case AttributeTag::FileAttributes: {
		auto res = sizeof(stat.attr);
		if(size >= res) {
			memcpy(buffer, &stat.attr, res);
		}
		return res;
	}

	case AttributeTag::Acl: {
		auto res = sizeof(stat.acl);
		if(size >= res) {
			memcpy(buffer, &stat.acl, res);
		}
		return res;
	}

	case AttributeTag::Compression: {
		auto res = sizeof(stat.compression);
		if(size >= res) {
			memcpy(buffer, &stat.compression, res);
		}
		return res;
	}

	default:
		return Error::NotFound;
	}
}

int FileSystem::fgetxattr(FileHandle file, AttributeTag tag, void* buffer, size_t size)
{
	Stat s{};
	int res = fstat(file, &s);
	if(res < 0) {
		return res;
	}

	return readAttribute(s, tag, buffer, size);
}

int FileSystem::getxattr(const char* path, AttributeTag tag, void* buffer, size_t size)
{
	Stat s{};
	int res = stat(path, &s);
	if(res < 0) {
		return res;
	}

	return readAttribute(s, tag, buffer, size);
}

} // namespace FWFS

} // namespace IFS
