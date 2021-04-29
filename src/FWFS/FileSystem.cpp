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

#include <IFS/FWFS/FileSystem.h>
#include <IFS/FWFS/Object.h>
#include <IFS/Util.h>

#ifdef DEBUG_FWFS
#include <Platform/Timers.h>
#endif

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
	if(!fd.isAllocated()) {                                                                                            \
		return Error::FileNotOpen;                                                                                     \
	}

/**
 * @brief Directories are enumerated using a regular file descriptor,
 * except they're dynamically allocated to avoid running out of handles when
 * parsing directory trees.
 */
using FileDir = FWFileDesc;

void FileSystem::printObject(const FWObjDesc& od, bool isChild)
{
#if DEBUG_VERBOSE_LEVEL >= DBG
	char name[260];
	if(od.obj.isRef()) {
		m_snprintf(name, sizeof(name), " REF #0x%08x", od.obj.getRef());
	} else if(od.obj.isNamed()) {
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
	m_printf("%s #0x%08X: %-5u bytes, type = 0x%02X %s, %s\r\n", isChild ? "    " : "", od.id, od.obj.size(),
			 od.obj.typeData, toString(od.obj.type()).c_str(), name);

	if(isChild || !od.obj.isNamed()) {
		return;
	}

	int res;
	FWObjDesc child;
	while((res = readChildObjectHeader(od, child)) >= 0) {
		printObject(child, true);
		child.next();
	}

#endif
}

/**
 * @brief initialise Stat structure and copy information from directory entry
 * @param stat
 * @param entry
 */
int FileSystem::fillStat(Stat& stat, const FWObjDesc& entry)
{
	assert(entry.obj.isNamed());

	stat = Stat{};
	stat.fs = this;
	stat.id = entry.id;
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
				res = getChildObject(entry, child, od);
				if(res < 0) {
					return res;
				}
				stat.size += od.obj.contentSize();
			} else {
				stat.size += child.obj.contentSize();
			}
			child.next();
			continue;
		}

		switch(child.obj.type()) {
		case Object::Type::ObjAttr: {
			Object::Attributes objattr = child.obj.data8.objectAttributes.attr;
			stat.attr |= getFileAttributes(objattr);
			break;
		}

		case Object::Type::Compression:
			stat.compression = child.obj.data8.compression;
			break;

		case Object::Type::ReadACE:
			stat.acl.readAccess = child.obj.data8.ace.role;
			break;

		case Object::Type::WriteACE:
			stat.acl.writeAccess = child.obj.data8.ace.role;
			break;

		default:; // Not interested
		}
		child.next();
	}

	switch(entry.obj.type()) {
	case Object::Type::Directory:
		stat.attr |= FileAttribute::Directory;
		break;
	case Object::Type::MountPoint:
		stat.attr |= FileAttribute::Directory + FileAttribute::MountPoint;
		break;
	default:;
	}

	checkStat(stat);

	return readObjectName(entry, stat.name);
}

int FileSystem::findUnusedDescriptor()
{
	for(int i = 0; i < FWFS_MAX_FDS; ++i) {
		if(!fileDescriptors[i].isAllocated()) {
			return i;
		}
	}

	return Error::OutOfFileDescs;
}

int FileSystem::read(FileHandle file, void* data, size_t size)
{
	GET_FD();

	if(fd.isMountPoint()) {
		return fd.fileSystem->read(fd.file, data, size);
	}

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
			res = getChildObject(fd.odFile, child, odData);
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

			if(res < 0 || readTotal == size || fd.cursor >= fd.dataSize) {
				// if(res < 0 || readTotal == size) {
				break;
			}
		}

		child.next();
	}

	return (res == FS_OK) || (res == Error::EndOfObjects) ? readTotal : res;
}

int FileSystem::write(FileHandle file, const void* data, size_t size)
{
	GET_FD();

	if(fd.isMountPoint()) {
		return fd.fileSystem->write(fd.file, data, size);
	}

	return Error::ReadOnly;
}

int FileSystem::lseek(FileHandle file, int offset, SeekOrigin origin)
{
	GET_FD();

	if(fd.isMountPoint()) {
		return fd.fileSystem->lseek(fd.file, offset, origin);
	}

	int newOffset = offset;
	if(origin == SeekOrigin::Current) {
		newOffset += int(fd.cursor);
	} else if(origin == SeekOrigin::End) {
		newOffset += int(fd.dataSize);
	}

	if(uint32_t(newOffset) > fd.dataSize) {
		return Error::SeekBounds;
	}

	fd.cursor = newOffset;
	return newOffset;
}

String FileSystem::getErrorString(int err)
{
	if(Error::isSystem(err)) {
		for(unsigned i = 0; i < FWFS_MAX_VOLUMES; ++i) {
			auto& fs = volumes[i].fileSystem;
			if(!fs) {
				continue;
			}
			String s = fs->getErrorString(err);
			if(s) {
				return s;
			}
		}
	}
	return Error::toString(err);
}

int FileSystem::setVolume(uint8_t index, IFileSystem* fileSystem)
{
	if(index >= FWFS_MAX_VOLUMES || fileSystem == this) {
		return Error::BadVolumeIndex;
	}

	volumes[index].fileSystem.reset(fileSystem);
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
	FWObjDesc odVolume{};
	FWObjDesc od{FWFS_BASE_OFFSET};
	int res;
	while((res = readObjectHeader(od)) >= 0) {
		++objectCount;
		printObject(od, false);

		if(od.obj.type() == Object::Type::Volume) {
			odVolume = od;
		} else if(od.obj.type() == Object::Type::Directory) {
			odRoot = od;
		} else if(od.obj.type() == Object::Type::End) {
			// @todo verify checksum
			// od.obj.end.checksum
			break;
		}

		od.next();
	}

	volume = odVolume.id;
	debug_d("Ended @ 0x%08X, %u objects, volume @ 0x%08X", od.id, objectCount, volume);

	if(res < 0) {
		return res;
	}

	if(volume == 0) {
		debug_e("Volume object missing");
		return Error::BadFileSystem;
	}

	FWObjDesc child;
	res = findChildObjectHeader(odVolume, child, Object::Type::Directory);
	if(res < 0) {
		debug_e("Root directory reference missing");
		return Error::BadFileSystem;
	}
	if(child.obj.getRef() != odRoot.id) {
		debug_e("Root directory is not last");
		return Error::BadFileSystem;
	}

	// Having scanned all the objects, check the end marker
	auto offset = od.nextOffset();
	if(!partition.read(offset, marker) || marker != FWFILESYS_END_MARKER) {
		debug_e("Filesys end marker invalid: found 0x%08x, expected 0x%08x", marker, FWFILESYS_END_MARKER);
		return Error::BadFileSystem;
	}

	Stat stat;
	fillStat(stat, odRoot);
	rootACL = stat.acl;

	flags[Flag::mounted] = true;

	return FS_OK;
}

int FileSystem::getinfo(Info& info)
{
	int res{FS_OK};

	info.clear();
	info.type = Type::FWFS;
	info.maxNameLength = 255;
	info.maxPathLength = INT16_MAX;
	info.attr = Attribute::ReadOnly;
	info.partition = partition;
	info.volumeSize = partition.size();

	if(isMounted()) {
		FWObjDesc odVolume;
		res = findObject(volume, odVolume);
		if(res >= 0) {
			info.creationTime = odVolume.obj.data16.named.mtime;
			readObjectName(odVolume, info.name);
			FWObjDesc od;
			if(findChildObjectHeader(odVolume, od, Object::Type::ID32) == FS_OK) {
				info.volumeID = od.obj.data8.id32.value;
			}
		}
		info.attr |= Attribute::Mounted;
	}

	return res;
}

int FileSystem::readObjectHeader(FWObjDesc& od)
{
	int res = partition.read(od.offset(), od.obj) ? FS_OK : Error::ReadFailure;

#if FWFS_DEBUG
	debug_d("read #0x%08x - %s", od.ref, toString(od.obj.type()).c_str());
#endif

	return res;
}

int FileSystem::getChildObject(const FWObjDesc& parent, const FWObjDesc& child, FWObjDesc& od)
{
	if(child.obj.isRef()) {
		int res = findObject(child.obj.getRef(), od);
		if(res == FS_OK && od.obj.type() != child.obj.type()) {
			// Reference must point to object of same type
			return Error::BadObject;
		}
		return res;
	}

	od = child;
	od.id += parent.childTableOffset();
	return FS_OK;
}

int FileSystem::findObject(Object::ID id, FWObjDesc& od)
{
	od.id = id;
	int res = readObjectHeader(od);
	if(res < 0) {
		return res;
	}
	if(od.obj.isRef()) {
		// Reference must point to an actual object, not another reference
		return Error::BadObject;
	}
	return FS_OK;
}

int FileSystem::readChildObjectHeader(const FWObjDesc& parent, FWObjDesc& child)
{
	assert(parent.obj.isNamed());

	if(child.id >= parent.obj.childTableSize()) {
		return Error::EndOfObjects;
	}

	// Get the absolute offset for the child object
	uint32_t tableOffset = parent.childTableOffset();
	child.id += tableOffset;
	int res = readObjectHeader(child);
	child.id -= tableOffset;
	return res;
}

int FileSystem::readObjectContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer)
{
	offset += od.contentOffset();
	return partition.read(offset, buffer, size) ? FS_OK : Error::ReadFailure;
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

int FileSystem::resolveMountPoint(const FWObjDesc& odMountPoint, IFileSystem*& fileSystem)
{
	assert(odMountPoint.obj.isMountPoint());

	FWObjDesc odVolumeIndex;
	int res = findChildObjectHeader(odMountPoint, odVolumeIndex, Object::Type::VolumeIndex);
	if(res < 0) {
		debug_e("Mount point missing volume index");
		return res;
	}

	auto index = odVolumeIndex.obj.data8.volumeIndex.index;
	if(index >= FWFS_MAX_VOLUMES) {
		return Error::BadVolumeIndex;
	}
	fileSystem = volumes[index].fileSystem.get();
	if(fileSystem == nullptr) {
		return Error::NotMounted;
	}

	return FS_OK;
}

int FileSystem::findLinkedObject(const char*& path, IFileSystem*& fileSystem)
{
	CHECK_MOUNTED();

	FWObjDesc od;
	int res = findObjectByPath(path, od);

	if(res < 0) {
		return res;
	}

	if(!od.obj.isMountPoint() || isRootPath(path)) {
		return Error::ReadOnly;
	}

	return resolveMountPoint(od, fileSystem);
}

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

	char buf[namelen];
	int res;
	FWObjDesc od;
	while((res = readChildObjectHeader(parent, od)) >= 0) {
		if(od.obj.isNamed()) {
			res = getChildObject(parent, od, child);
			if(res < 0) {
				break;
			}

			auto& objNamed = child.obj.data16.named;
			if(objNamed.namelen == namelen) {
				if(namelen == 0) {
					break;
				}
				res = readObjectContent(child, objNamed.nameOffset(), namelen, buf);
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

int FileSystem::opendir(const char* path, DirHandle& dir)
{
	CHECK_MOUNTED();

	FS_CHECK_PATH(path)

	FWObjDesc od;
	int res = findObjectByPath(path, od);
	if(res < 0) {
		return res;
	}

	auto fd = new FileDir{od};

	if(od.obj.isMountPoint()) {
		res = resolveMountPoint(od, fd->fileSystem);
		if(res < 0) {
			delete fd;
			return res;
		}
		res = fd->fileSystem->opendir(path, fd->dir);
		if(res < 0) {
			delete fd;
			return res;
		}
	}

	dir = DirHandle(fd);
	return FS_OK;
}

/* Reading a directory simply gets details about any child named objects.
 * The XXXdir() methods will work on any named objects, including files, although
 * these don't normally contain named children.
 */
int FileSystem::readdir(DirHandle dir, Stat& stat)
{
	GET_FILEDIR()
	auto& fd = *d;

	if(fd.isMountPoint()) {
		return fd.fileSystem->readdir(fd.dir, stat);
	}

	FWObjDesc odDir = fd.odFile;
	odDir = fd.odFile;

	FWObjDesc od{fd.cursor};
	int res;
	while((res = readChildObjectHeader(odDir, od)) >= 0) {
		if(od.obj.isNamed()) {
			FWObjDesc child;
			res = getChildObject(odDir, od, child);
			if(res >= 0) {
				res = fillStat(stat, child);
			}
			if(od.obj.isMountPoint()) {
				stat.attr += FileAttribute::MountPoint + FileAttribute::Directory;
			}
			od.next();
			break;
		}

		od.next();
	}

	fd.cursor = od.offset();

	return res == Error::EndOfObjects ? Error::NoMoreFiles : res;
}

int FileSystem::rewinddir(DirHandle dir)
{
	GET_FILEDIR()
	auto& fd = *d;

	if(fd.isMountPoint()) {
		return fd.fileSystem->rewinddir(fd.dir);
	}

	fd.cursor = 0;
	return 0;
}

int FileSystem::closedir(DirHandle dir)
{
	GET_FILEDIR()
	auto fd = d;

	int res{FS_OK};
	if(fd->isMountPoint()) {
		res = fd->fileSystem->closedir(fd->dir);
	}
	delete fd;
	return res;
}

int FileSystem::mkdir(const char* path)
{
	IFileSystem* fs;
	int res = findLinkedObject(path, fs);
	if(res < 0) {
		return res;
	}
	if(*path == '\0') {
		return FS_OK;
	}
	return fs->mkdir(path);
}

/** @brief find object by path
 *  @param path full path for object, OUT: If a link is encountered the path tail will be returned.
 *  @param od descriptor for located object
 *  @retval error code
 *  @note specifying an empty string or nullptr for path will fetch the root directory object
 *  Called by open, opendir and stat methods.
 *  @todo track ACEs during path traversal and store resultant ACL in file descriptor.
 */
int FileSystem::findObjectByPath(const char*& path, FWObjDesc& od)
{
	// Start with the root directory object
	od = odRoot;

	// Empty paths indicate root directory
	const char* tail = path;
	FS_CHECK_PATH(tail);
	if(tail == nullptr) {
		return FS_OK;
	}

	int res{FS_OK};
	const char* sep;
	do {
		sep = strchr(tail, '/');
		size_t namelen;
		if(sep == nullptr) {
			namelen = strlen(tail);
		} else {
			namelen = sep - tail;
			++path;
		}
		path += namelen;

		FWObjDesc parent = od;
		res = findChildObject(parent, od, tail, namelen);

		if(res < 0) {
			break;
		}

		tail = path;
	} while(sep != nullptr && !od.obj.isMountPoint());

	return res;
}

int FileSystem::getObjectDataSize(FWObjDesc& od, size_t& dataSize)
{
	dataSize = 0;
	int res;
	FWObjDesc child;
	while((res = readChildObjectHeader(od, child)) >= 0) {
		if(child.obj.isData()) {
			FWObjDesc odData;
			res = getChildObject(od, child, odData);
			if(res < 0) {
				return res;
			}

			dataSize += odData.obj.contentSize();
		}

		child.next();
	}

	return FS_OK;
}

FileHandle FileSystem::open(const char* path, OpenFlags flags)
{
	CHECK_MOUNTED();

#if FWFS_DEBUG
	debug_d("open('%s', %s)", path, toString(flags).c_str());
#endif

	FS_CHECK_PATH(path)

	FWObjDesc od;
	int res = findObjectByPath(path, od);
	if(res < 0) {
		return res;
	}

#if FWFS_DEBUG
	debug_d("Found '%s' @ 0x%08x", path, od.ref);
#endif

	int descriptorIndex = findUnusedDescriptor();
	if(descriptorIndex < 0) {
		return descriptorIndex;
	}

	auto& fd = fileDescriptors[descriptorIndex];
	fd = FWFileDesc{od};

	bool openMountPoint = od.obj.isMountPoint();
	if(openMountPoint && flags[OpenFlag::NoFollow]) {
		if(path == nullptr || *path == '\0') {
			openMountPoint = false;
			// Change to regular directory...
			fd.odFile.obj.typeData = unsigned(Object::Type::Directory);
		}
	}

	if(openMountPoint) {
		res = resolveMountPoint(od, fd.fileSystem);
		if(res == FS_OK) {
			res = fd.file = fd.fileSystem->open(path, flags);
		}
	} else if(flags[OpenFlag::Write]) {
		res = Error::ReadOnly;
	} else {
		res = getObjectDataSize(fd.odFile, fd.dataSize);
	}

	if(res < 0) {
		fd.reset();
		return res;
	}

#if FWFS_DEBUG
	debug_d("Descriptor #%u allocated", descriptorIndex);
#endif

	return FWFS_HANDLE_MIN + descriptorIndex;
}

int FileSystem::close(FileHandle file)
{
	GET_FD();

#if FWFS_DEBUG
	debug_d("descriptor #%u close", file - FWFS_HANDLE_MIN);
#endif

	int res{FS_OK};
	if(fd.isMountPoint()) {
		res = fd.fileSystem->close(fd.file);
	}

	fd.reset();
	return res;
}

int FileSystem::stat(const char* path, Stat* stat)
{
	CHECK_MOUNTED();

	FWObjDesc od;
	int res = findObjectByPath(path, od);

	if(res < 0) {
		return res;
	}

	if(od.obj.isMountPoint() && !isRootPath(path)) {
		IFileSystem* fs;
		res = resolveMountPoint(od, fs);
		return (res < 0) ? res : fs->stat(path, stat);
	}

	if(stat) {
		res = fillStat(*stat, od);
	}

	return res;
}

int FileSystem::fstat(FileHandle file, Stat* stat)
{
	GET_FD();

	if(fd.isMountPoint()) {
		return fd.fileSystem->fstat(fd.file, stat);
	}

	if(stat == nullptr) {
		return Error::BadParam;
	}

	return fillStat(*stat, fd.odFile);
}

int FileSystem::fcontrol(FileHandle file, ControlCode code, void* buffer, size_t bufSize)
{
	GET_FD()

	if(fd.isMountPoint()) {
		return fd.fileSystem->fcontrol(fd.file, code, buffer, bufSize);
	}

	switch(code) {
	case FCNTL_GET_MD5_HASH:
		return getMd5Hash(fd, buffer, bufSize);
	default:
		return Error::NotSupported;
	}
}

int FileSystem::eof(FileHandle file)
{
	GET_FD();

	if(fd.isMountPoint()) {
		return fd.fileSystem->eof(fd.file);
	}

	// 0 - not EOF, > 0 - at EOF, < 0 - error
	return fd.cursor >= fd.dataSize ? 1 : 0;
}

int32_t FileSystem::tell(FileHandle file)
{
	GET_FD();

	if(fd.isMountPoint()) {
		return fd.fileSystem->tell(fd.file);
	}

	return fd.cursor;
}

int FileSystem::getMd5Hash(FWFileDesc& fd, void* buffer, size_t bufSize)
{
	constexpr size_t md5HashSize{16};
	if(bufSize < md5HashSize) {
		return Error::BadParam;
	}

	FWObjDesc child;
	int res = findChildObjectHeader(fd.odFile, child, Object::Type::Md5Hash);
	if(res < 0) {
		return res;
	}

	if(child.obj.contentSize() != md5HashSize) {
		return Error::BadObject;
	}
	FWObjDesc od;
	getChildObject(fd.odFile, child, od);

	res = readObjectContent(od, 0, md5HashSize, buffer);
	return (res < 0) ? res : md5HashSize;
}

int FileSystem::readAttribute(FWObjDesc& od, AttributeTag tag, void* buffer, size_t size)
{
	assert(od.obj.isNamed());

	auto setValue = [&](const void* value, size_t attrsize) -> int {
		memcpy(buffer, value, std::min(size, attrsize));
		return attrsize;
	};

	auto getChild = [&](Object::Type type, size_t attrsize) -> int {
		FWObjDesc child;
		int res = findChildObjectHeader(od, child, type);
		if(res < 0) {
			return res;
		}
		return setValue(&child.obj.typeData + child.obj.data8.contentOffset(), attrsize);
	};

	switch(tag) {
	case AttributeTag::ModifiedTime:
		return setValue(&od.obj.data16.named.mtime, sizeof(TimeStamp));
	case AttributeTag::FileAttributes: {
		FWObjDesc child;
		int res = findChildObjectHeader(od, child, Object::Type::ObjAttr);
		if(res < 0) {
			return res;
		}
		Object::Attributes objattr = child.obj.data8.objectAttributes.attr;
		FileAttributes fileAttr = getFileAttributes(objattr);
		return setValue(&fileAttr, sizeof(fileAttr));
	}
	case AttributeTag::ReadAce:
		return getChild(Object::Type::ReadACE, sizeof(UserRole));
	case AttributeTag::WriteAce:
		return getChild(Object::Type::WriteACE, sizeof(UserRole));
	case AttributeTag::Compression:
		return getChild(Object::Type::Compression, sizeof(Compression));
	default:
		return Error::NotFound;
	}
}

int FileSystem::fsetxattr(FileHandle file, AttributeTag tag, const void* data, size_t size)
{
	GET_FD();

	if(fd.isMountPoint()) {
		return fd.fileSystem->fsetxattr(fd.file, tag, data, size);
	}

	return Error::ReadOnly;
}

int FileSystem::fgetxattr(FileHandle file, AttributeTag tag, void* buffer, size_t size)
{
	GET_FD()

	if(fd.isMountPoint()) {
		return fd.fileSystem->fgetxattr(fd.file, tag, buffer, size);
	}

	return readAttribute(fd.odFile, tag, buffer, size);
}

int FileSystem::fenumxattr(FileHandle file, AttributeEnumCallback callback, void* buffer, size_t bufsize)
{
	GET_FD()

	if(fd.isMountPoint()) {
		return fd.fileSystem->fenumxattr(fd.file, callback, buffer, bufsize);
	}

	unsigned count{0};
	AttributeEnum e{buffer, bufsize};

	auto send = [&](AttributeTag tag, const void* value, size_t attrsize) -> bool {
		++count;
		e.set(tag, value, attrsize);
		return callback(e);
	};

	if(!send(AttributeTag::ModifiedTime, &fd.odFile.obj.data16.named.mtime, sizeof(TimeStamp))) {
		return count;
	}

	int res;
	FWObjDesc child;
	bool cont{true};
	while(cont && (res = readChildObjectHeader(fd.odFile, child)) >= 0) {
		switch(child.obj.type()) {
		case Object::Type::ObjAttr: {
			Object::Attributes objattr = child.obj.data8.objectAttributes.attr;
			auto attr = getFileAttributes(objattr);
			cont = send(AttributeTag::FileAttributes, &attr, sizeof(attr));
			break;
		}

		case Object::Type::Compression:
			cont = send(AttributeTag::Compression, &child.obj.data8.compression, sizeof(Compression));
			break;

		case Object::Type::ReadACE:
			cont = send(AttributeTag::ReadAce, &child.obj.data8.ace.role, sizeof(UserRole));
			break;

		case Object::Type::WriteACE:
			cont = send(AttributeTag::WriteAce, &child.obj.data8.ace.role, sizeof(UserRole));
			break;

		case Object::Type::VolumeIndex:
			cont = send(AttributeTag::VolumeIndex, &child.obj.data8.volumeIndex, sizeof(uint8_t));
			break;

		case Object::Type::Md5Hash: {
			e.attrsize = 16;
			FWObjDesc od;
			getChildObject(fd.odFile, child, od);
			e.tag = AttributeTag::Md5Hash;
			e.size = std::min(e.attrsize, e.bufsize);
			readObjectContent(od, 0, e.size, e.buffer);
			++count;
			cont = callback(e);
			break;
		}

		case Object::Type::Data8:
		case Object::Type::Data16:
		case Object::Type::Data24:
			break; // ignore

		default:
			if(!child.obj.isNamed()) {
				debug_w("[FWFS] Ignoring unknown object %u (%u bytes)", child.obj.type(), child.obj.size());
			}
		}

		child.next();
	}

	return count;
}

int FileSystem::setxattr(const char* path, AttributeTag tag, const void* data, size_t size)
{
	IFileSystem* fs;
	int res = findLinkedObject(path, fs);
	return (res < 0) ? res : fs->setxattr(path, tag, data, size);
}

int FileSystem::getxattr(const char* path, AttributeTag tag, void* buffer, size_t size)
{
	CHECK_MOUNTED();

	FWObjDesc od;
	int res = findObjectByPath(path, od);

	if(res < 0) {
		return res;
	}

	if(od.obj.isMountPoint() && !isRootPath(path)) {
		IFileSystem* fs;
		res = resolveMountPoint(od, fs);
		return (res < 0) ? res : fs->getxattr(path, tag, buffer, size);
	}

	return readAttribute(od, tag, buffer, size);
}

int FileSystem::ftruncate(FileHandle file, size_t new_size)
{
	GET_FD();

	if(fd.isMountPoint()) {
		return fd.fileSystem->ftruncate(fd.file, new_size);
	}

	return Error::ReadOnly;
}

int FileSystem::flush(FileHandle file)
{
	GET_FD();

	if(fd.isMountPoint()) {
		return fd.fileSystem->flush(fd.file);
	}

	return Error::ReadOnly;
}

int FileSystem::rename(const char* oldpath, const char* newpath)
{
	IFileSystem* fsOld;
	int res = findLinkedObject(oldpath, fsOld);
	if(res < 0) {
		return res;
	}
	IFileSystem* fsNew;
	res = findLinkedObject(newpath, fsNew);
	if(res < 0) {
		return res;
	}
	// Cross-volume renaming not supported
	if(fsOld != fsNew) {
		return Error::NotSupported;
	}
	return fsOld->rename(oldpath, newpath);
}

int FileSystem::remove(const char* path)
{
	IFileSystem* fs;
	int res = findLinkedObject(path, fs);
	return (res < 0) ? res : fs->remove(path);
}

int FileSystem::fremove(FileHandle file)
{
	GET_FD();

	if(fd.isMountPoint()) {
		return fd.fileSystem->fremove(fd.file);
	}

	return Error::ReadOnly;
}

} // namespace FWFS
} // namespace IFS
