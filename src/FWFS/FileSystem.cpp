/*
 * FileSystem.cpp
 *
 *  Created on: 19 Jul 2018
 *      Author: mikee47
 *
 */

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
	if(!volumes[0].isMounted()) {                                                                                      \
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

/** @brief initialise FileStat structure and copy information from directory entry
 *  @param stat
 *  @param entry
 */
int FileSystem::fillStat(FileStat& stat, const FWObjDesc& entry)
{
	assert(entry.obj.isNamed());

	stat = FileStat{};
	stat.fs = this;
	stat.id = entry.ref.fileID();
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
				closeObject(od);
			} else {
				stat.size += child.obj.contentSize();
			}
		} else {
			switch(child.obj.type()) {
			case Object::Type::ObjAttr: {
				Object::Attributes attr = child.obj.data8.objectAttributes.attr;
				if(attr[Object::Attribute::ReadOnly]) {
					stat.attr |= File::Attribute::ReadOnly;
				}
				if(attr[Object::Attribute::Archive]) {
					stat.attr |= File::Attribute::Archive;
				}
				break;
			}

			case Object::Type::ObjectStore:
				stat.attr |= File::Attribute::MountPoint | File::Attribute::Directory;
				break;

			case Object::Type::Compression:
				stat.attr |= File::Attribute::Compressed;
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
		stat.attr |= File::Attribute::Directory;
	}

	if(!stat.attr[File::Attribute::Compressed]) {
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

int FileSystem::read(File::Handle file, void* data, size_t size)
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

			closeObject(odData);

			if(res < 0 || readTotal == size) {
				break;
			}
		}

		child.next();
	}

	return (res == FS_OK) || (res == Error::EndOfObjects) ? readTotal : res;
}

int FileSystem::lseek(File::Handle file, int offset, SeekOrigin origin)
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

int FileSystem::setVolume(uint8_t num, IObjectStore* store)
{
	if(num >= FWFS_MAX_VOLUMES) {
		return Error::BadStore;
	}

	auto& vol = volumes[num];
	delete vol.store;
	vol.store = store;
	vol.ref.storenum = num;
	return FS_OK;
}

int FileSystem::mount()
{
	// Store #0 is mandatory
	auto& vol = volumes[0];
	int res = mountVolume(vol);
	if(res < 0) {
		return res;
	}

	FWObjDesc odRoot;
	res = openRootObject(vol, odRoot);
	if(res < 0) {
		return res;
	}
	FileStat stat;
	fillStat(stat, odRoot);
	rootACL = stat.acl;
	closeObject(odRoot);

	// Other volumes are mounted as required

	return FS_OK;
}

int FileSystem::mountVolume(FWVolume& volume)
{
	if(volume.store == nullptr) {
		return Error::StoreNotMounted;
	}

	int res = volume.store->initialise();
	if(res < 0) {
		return res;
	}

	unsigned objectCount = 0;
	FWObjDesc od;
	od.ref = volume.ref;
	while((res = readObjectHeader(od)) >= 0) {
		++objectCount;
		//		printObject(od);

		if(od.obj.type() == Object::Type::Volume) {
			volume.ref = od.ref;
		} else if(od.obj.type() == Object::Type::End) {
			// @todo verify checksum
			// od.obj.end.checksum
			break;
		}

		od.next();
	}

	debug_d("Ended @ 0x%08X (#%u), %u objects, volume @ 0x%08X, od @ 0x%08X", od.ref.offset, od.ref.offset, od.ref.id,
			objectCount, volume.ref.offset);

	if(res < 0) {
		return res;
	}

	//	if(!_rootDirectory.isValid()) {
	//		debug_e("root directory missing");
	//		return Error::BadFileSystem;
	//	}

	// Having scanned all the objects, let the store know where the end is
	res = volume.store->mounted(od);

	return res;
}

int FileSystem::openRootObject(const FWVolume& volume, FWObjDesc& odRoot)
{
	FWObjDesc odVolume(volume.ref);
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

	auto& volume = volumes[0];

	info.clear();
	info.type = Type::FWFS;
	info.maxNameLength = INT16_MAX;
	info.maxPathLength = INT16_MAX;
	info.attr = Attribute::ReadOnly;
	if(volume.store) {
		info.partition = volume.store->getPartition();
		info.volumeSize = info.partition.size();
	}

	if(volume.isMounted()) {
		FWObjDesc odVolume(volume.ref);
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

int FileSystem::resolveMountPoint(const FWObjDesc& odMountPoint, FWObjDesc& odResolved)
{
	assert(odMountPoint.obj.isMountPoint());

	FWObjDesc odStore;
	int res = findChildObjectHeader(odMountPoint, odStore, Object::Type::ObjectStore);
	if(res < 0) {
		debug_e("Mount point missing object store");
		return res;
	}

	// Volume #0 is the primary and already mounted, also avoid circular references
	auto storenum = odStore.obj.data8.objectStore.storenum;
	if(storenum == 0 || storenum == odMountPoint.ref.storenum || storenum >= FWFS_MAX_VOLUMES) {
		return Error::BadStore;
	}

	auto& vol = volumes[storenum];
	if(!vol.isMounted()) {
		res = mountVolume(vol);
		if(res < 0) {
			return res;
		}
	}

	// Locate the root directory
	return openRootObject(vol, odResolved);
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

			closeObject(child);
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
 *  @retval File::Handle file handle, or error code
 */
File::Handle FileSystem::allocateFileDescriptor(FWObjDesc& odFile)
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

			closeObject(odData);
		}

		child.next();
	}

	// We now own the descriptor
	odFile.ref.refCount = 0;

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
		File::Handle handle = allocateFileDescriptor(od);
		if(handle < 0) {
			res = handle;
			closeObject(od);
		} else {
			dir = reinterpret_cast<DirHandle>(handle);
		}
	}

	return res;
}

/*
 * TODO: To implement this we could do with storing the directory offset in `stat`.
 */
int FileSystem::fopendir(const FileStat* stat, DirHandle& dir)
{
	return Error::NotImplemented;
	/*

	CHECK_MOUNTED();
	if(dir == nullptr) {
		return Error::BadParam;
	}

	FWObjDesc od;
	int res = findObject
	int res = findObjectByPath(path, od);
	if(res >= 0) {
		File::Handle handle = allocateFileDescriptor(od);
		if(handle < 0) {
			res = handle;
			closeObject(od);
		} else {
			*dir = reinterpret_cast<DirHandle>(handle);
		}
	}

	return res;
*/
}

/* Reading a directory simply gets details about any child named objects.
 * The XXXdir() methods will work on any named objects, including files, although
 * these don't normally contain named children.
 */
int FileSystem::readdir(DirHandle dir, FileStat& stat)
{
	CHECK_MOUNTED();

	auto file = reinterpret_cast<int>(dir);
	GET_FD();

	int res;
	FWObjDesc odDir;
	if(fd.odFile.obj.isMountPoint()) {
		res = resolveMountPoint(fd.odFile, odDir);
		if(res < 0)
			return res;
	} else {
		odDir = fd.odFile;
		odDir.ref.refCount = 0;
	}

	ObjRef ref;
	ref.offset = fd.cursor;
	FWObjDesc od(ref);
	while((res = readChildObjectHeader(odDir, od)) >= 0) {
		if(od.obj.isNamed()) {
			FWObjDesc child;
			res = openChildObject(odDir, od, child);
			if(res >= 0) {
				res = fillStat(stat, child);
				closeObject(child);
			}
			od.next();
			break;
		}

		od.next();
	}

	fd.cursor = od.ref.offset;

	if(fd.odFile.obj.isMountPoint()) {
		closeObject(odDir);
	}

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
	int res = openRootObject(volumes[0], od);
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
		od.ref.refCount = 0;
		res = findChildObject(parent, od, tail, namelen);
		closeObject(parent);

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

File::Handle FileSystem::fopen(const FileStat& stat, File::OpenFlags flags)
{
	CHECK_MOUNTED();
	if(stat.fs != this) {
		return Error::BadParam;
	}

	FWObjDesc od(stat.id);
	int res = openObject(od);
	if(res >= 0) {
		res = allocateFileDescriptor(od);
		if(res < 0) {
			closeObject(od);
		}
	}

	return res;
}

File::Handle FileSystem::open(const char* path, File::OpenFlags flags)
{
	CHECK_MOUNTED();

	debug_d("open('%s', 0x%02X)", path, flags);

	FWObjDesc od;
	int res = findObjectByPath(path, od);
	if(res >= 0) {
		res = allocateFileDescriptor(od);
		if(res < 0) {
			closeObject(od);
		}
	}

	return res;
}

int FileSystem::close(File::Handle file)
{
	GET_FD();

	debug_d("descriptor #%u close, refCount = %u", file - FWFS_HANDLE_MIN, fd.odFile.ref.refCount);
	assert(fd.odFile.ref.refCount == 1);

	closeObject(fd.odFile);
	fd.attr = 0;
	return FS_OK;
}

int FileSystem::stat(const char* path, FileStat* stat)
{
	CHECK_MOUNTED();

	FWObjDesc od;
	int res = findObjectByPath(path, od);
	if(res >= 0) {
		if(stat) {
			res = fillStat(*stat, od);
		}
		closeObject(od);
	}
	return res;
}

int FileSystem::fstat(File::Handle file, FileStat* stat)
{
	GET_FD();

	return stat ? fillStat(*stat, fd.odFile) : Error::BadParam;
}

int FileSystem::fcontrol(File::Handle file, ControlCode code, void* buffer, size_t bufSize)
{
	switch(code) {
	case FCNTL_GET_MD5_HASH:
		return getMd5Hash(file, buffer, bufSize);
	default:
		return Error::NotSupported;
	}
}

int FileSystem::eof(File::Handle file)
{
	GET_FD();

	// 0 - not EOF, > 0 - at EOF, < 0 - error
	return fd.cursor >= fd.dataSize ? 1 : 0;
}

int32_t FileSystem::tell(File::Handle file)
{
	GET_FD();

	return fd.cursor;
}

/*
 * fileid identifies the object store and object identifier, but it doesn't implicitly tell
 * us the path. Whilst we can have aliases and such, we're interested in the 'true' path of the
 * file object. So, we start at the root and iterate through every single object on the system
 * searching for the file. Not pretty, but then the only reason this method is provided is
 * for the hybrid filesystem, which needs to match full file paths across filesystems.
 */
int FileSystem::getFilePath(File::ID fileid, NameBuffer& path)
{
	int res = FS_OK;

	auto& vol = volumes[0];

	// Could be the root directory object itself
	if(vol.ref.fileID() == fileid) {
		path.length = 0;
	} else {
		// Start searching from root directory
		FWObjDesc parent;
		res = openRootObject(vol, parent);
		if(res >= 0) {
			res = seekFilePath(parent, fileid, path);
		}
	}
	path.terminate();

	if(res >= 0 && path.overflow()) {
		res = Error::BufferTooSmall;
	}

	debug_d("getFilePath(%u) returned %d, '%s'", fileid, res, path.buffer);

	return res;
}

/*
 * @param parent this object is closed before returning
 * @param fileid
 * @param path
 * @retval error code
 * @note name buffer overrun does not return an error; we check for this in getFilePath()
 */
int FileSystem::seekFilePath(FWObjDesc& parent, File::ID fileid, NameBuffer& path)
{
	auto addPathSeg = [&](FWObjDesc& child) {
		path.addSep();
		NameBuffer name(path.endptr(), path.space());
		int res = readObjectName(child, name);
		path.length += name.length;
		return res;
	};

	FWObjDesc child;
	int res;
	while((res = readChildObjectHeader(parent, child)) >= 0) {
		if(!child.obj.isNamed()) {
			child.next();
			continue;
		}

		FWObjDesc od;
		res = openChildObject(parent, child, od);
		child.next();

		if(res < 0) {
			continue;
		}

		//		debug_d("obj #%u, type = %u, namelen = %u", od.ref.id, od.obj.type(), od.obj.data16.named.namelen);

		if(od.ref.fileID() == fileid) {
			// Success!
			res = addPathSeg(od);
			closeObject(od);
			break;
		}

		if(!od.obj.isDir() && !od.obj.isMountPoint()) {
			closeObject(od);
			continue;
		}

		auto pathlen = path.length;
		res = addPathSeg(od);
		if(res < 0) {
			closeObject(od);
			break;
		}

		if(od.obj.isMountPoint()) {
			FWObjDesc mp;
			res = resolveMountPoint(od, mp);
			if(res >= 0) {
				closeObject(od);
				res = seekFilePath(mp, fileid, path);
			}
		} else {
			res = seekFilePath(od, fileid, path);
		}

		if(res == FS_OK) {
			break; // Success!
		}

		// Not found in this directory, keep looking
		path.length = pathlen;
	}

	closeObject(parent);

	return res == Error::EndOfObjects ? Error::NotFound : res;
}

int FileSystem::getMd5Hash(File::Handle file, void* buffer, size_t bufSize)
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
	closeObject(od);

	return md5HashSize;
}

} // namespace FWFS

} // namespace IFS
