/*
 * FirmwareFileSystem.cpp
 *
 *  Created on: 19 Jul 2018
 *      Author: mikee47
 *
 */

#include <IFS/FirmwareFileSystem.h>
#include <IFS/FWFileDefs.h>
#include <IFS/IFSFlashMedia.h>
#include <IFS/IFSUtil.h>

/*
 * Macros to perform standard checks
 */

#define CHECK_MOUNTED()                                                                                                \
	if(!_volumes[0].isMounted()) {                                                                                     \
		return FSERR_NotMounted;                                                                                       \
	}

#define GET_FD()                                                                                                       \
	CHECK_MOUNTED()                                                                                                    \
	if(file < FWFS_HANDLE_MIN || file > FWFS_HANDLE_MAX) {                                                             \
		return FSERR_InvalidHandle;                                                                                    \
	}                                                                                                                  \
	auto& fd = _fds[file - FWFS_HANDLE_MIN];                                                                           \
	if(!bitRead(fd.attr, fwfda_Allocated)) {                                                                           \
		return FSERR_FileNotOpen;                                                                                      \
	}

void FirmwareFileSystem::printObject(const FWObjDesc& od)
{
	char idstr[20];
	fwfsObjectTypeName(od.obj.type(), idstr, sizeof(idstr));
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
	debug_d("@0x%08X #%u: id = 0x%02X, %u bytes - %s%s", od.ref.offset, od.ref.id, od.obj._type, od.obj.size(), idstr,
			name);
	//	if (od.obj.size() <= 4)
	//		debug_hex(INFO, "OBJ", &od.obj, od.obj.size());
}

/** @brief initialise FileStat structure and copy information from directory entry
 *  @param stat
 *  @param entry
 */
int FirmwareFileSystem::fillStat(FileStat& stat, const FWObjDesc& entry)
{
	assert(entry.obj.isNamed());

	stat.clear();
	stat.fs = this;
	stat.id = entry.ref.fileID();
	stat.mtime = entry.obj.data16.named.mtime;
	stat.acl = _rootACL;
	stat.size = 0;

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
			case fwobt_FileAttr:
				stat.attr |= child.obj.data8.fileAttributes.attr;
				break;

			case fwobt_ObjectStore:
				stat.attr |= _BV(FileAttr::MountPoint) | _BV(FileAttr::Directory);
				break;

			case fwobt_Compression:
				stat.compression = child.obj.data8.compression.type;
				bitSet(stat.attr, FileAttr::Compressed);
				break;

			case fwobt_ReadACE:
				stat.acl.readAccess = child.obj.data8.ace.role;
				break;

			case fwobt_WriteACE:
				stat.acl.writeAccess = child.obj.data8.ace.role;
				break;

			default:; // Not interested
			}
		}

		child.next();
	}

	if(entry.obj.type() == fwobt_Directory) {
		bitSet(stat.attr, FileAttr::Directory);
	}

	return readObjectName(entry, stat.name);
}

int FirmwareFileSystem::findUnusedDescriptor()
{
	for(int i = 0; i < FWFS_MAX_FDS; ++i) {
		if(!bitRead(_fds[i].attr, fwfda_Allocated)) {
			return i;
		}
	}

	return FSERR_OutOfFileDescs;
}

int FirmwareFileSystem::read(file_t file, void* data, size_t size)
{
	GET_FD();

	uint32_t readTotal = 0;
	// Offset from start of data content
	FSExtent ext;

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

	return (res == FS_OK) || (res == FSERR_EndOfObjects) ? readTotal : res;
}

int FirmwareFileSystem::lseek(file_t file, int offset, SeekOriginFlags origin)
{
	GET_FD();

	int newOffset = offset;
	if(origin == eSO_CurrentPos) {
		newOffset += (int)fd.cursor;
	} else if(origin == eSO_FileEnd) {
		newOffset += (int)fd.dataSize;
	}

	debug_d("lseek(%d, %d, %d): %d", file, offset, origin, newOffset);

	if((uint32_t)newOffset > fd.dataSize) {
		return FSERR_SeekBounds;
	}

	fd.cursor = newOffset;
	return newOffset;
}

int FirmwareFileSystem::setVolume(uint8_t num, IFSObjectStore* store)
{
	if(num >= FWFS_MAX_VOLUMES) {
		return FSERR_BadStore;
	}

	auto& vol = _volumes[num];
	delete vol.store;
	vol.store = store;
	vol.ref.storenum = num;
	return FS_OK;
}

int FirmwareFileSystem::mount()
{
	// Store #0 is mandatory
	auto& vol = _volumes[0];
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
	_rootACL = stat.acl;
	closeObject(odRoot);

	// Other volumes are mounted as required

	return FS_OK;
}

int FirmwareFileSystem::mountVolume(FWVolume& volume)
{
	if(volume.store == nullptr) {
		return FSERR_StoreNotMounted;
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

		if(od.obj.type() == fwobt_Volume) {
			volume.ref = od.ref;
		} else if(od.obj.type() == fwobt_End) {
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
	//		return FSERR_BadFileSystem;
	//	}

	// Having scanned all the objects, let the store know where the end is
	res = volume.store->mounted(od);

	return res;
}

int FirmwareFileSystem::openRootObject(const FWVolume& volume, FWObjDesc& odRoot)
{
	FWObjDesc odVolume(volume.ref);
	int res = readObjectHeader(odVolume);
	if(res >= 0) {
		FWObjDesc child;
		res = findChildObjectHeader(odVolume, child, fwobt_Directory);
		if(res >= 0) {
			res = openChildObject(odVolume, child, odRoot);
		}
	}

	if(res < 0) {
		debug_e("Problem reading root directory %d", res);
	}

	return res;
}

int FirmwareFileSystem::getinfo(FileSystemInfo& info)
{
	int res = FS_OK;

	auto& volume = _volumes[0];

	info.clear();
	info.type = FileSystemType::FWFS;
	info.attr = _BV(FileAttr::ReadOnly);
	if(volume.store) {
		info.media = volume.store->getMedia();
		info.volumeSize = info.media->mediaSize();
	}

	if(volume.isMounted()) {
		FWObjDesc odVolume(volume.ref);
		res = readObjectHeader(odVolume);
		if(res >= 0) {
			readObjectName(odVolume, info.name);
			FWObjDesc od;
			if(findChildObjectHeader(odVolume, od, fwobt_ID32)) {
				info.volumeID = od.obj.data8.id32.value;
			}
		}
		bitSet(info.attr, FileSystemAttr::Mounted);
	}

	return res;
}

int FirmwareFileSystem::findChildObjectHeader(const FWObjDesc& parent, FWObjDesc& child, FWFS_ObjectType objId)
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

	return res == FSERR_EndOfObjects ? FSERR_NotFound : res;
}

int FirmwareFileSystem::resolveMountPoint(const FWObjDesc& odMountPoint, FWObjDesc& odResolved)
{
	assert(odMountPoint.obj.isMountPoint());

	FWObjDesc odStore;
	int res = findChildObjectHeader(odMountPoint, odStore, fwobt_ObjectStore);
	if(res < 0) {
		debug_e("Mount point missing object store");
		return res;
	}

	// Volume #0 is the primary and already mounted, also avoid circular references
	auto storenum = odStore.obj.data8.objectStore.storenum;
	if(storenum == 0 || storenum == odMountPoint.ref.storenum || storenum >= FWFS_MAX_VOLUMES) {
		return FSERR_BadStore;
	}

	auto& vol = _volumes[storenum];
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
int FirmwareFileSystem::findChildObject(const FWObjDesc& parent, FWObjDesc& child, const char* name, unsigned namelen)
{
	assert(parent.obj.isNamed());

	char buf[ALIGNUP(namelen)];
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
				res = readObjectContent(child, objNamed.nameOffset(), ALIGNUP(namelen), buf);
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

	return res == FSERR_EndOfObjects ? FSERR_NotFound : res;
}

/** @brief read an object name into a buffer, which might be smaller than required.
 *  @param od
 *  @param buffer
 *  @param bufSize
 *  @retval int error code
 *  @note the name is always nul-terminated
 */
int FirmwareFileSystem::readObjectName(const FWObjDesc& od, NameBuffer& name)
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
 *  @retval file_t file handle, or error code
 */
file_t FirmwareFileSystem::allocateFileDescriptor(FWObjDesc& odFile)
{
	int descriptorIndex = findUnusedDescriptor();
	if(descriptorIndex < 0) {
		return descriptorIndex;
	}

	auto& fd = _fds[descriptorIndex];

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

	bitSet(fd.attr, fwfda_Allocated);
	return FWFS_HANDLE_MIN + descriptorIndex;
}

int FirmwareFileSystem::opendir(const char* path, filedir_t* dir)
{
	CHECK_MOUNTED();
	if(dir == nullptr) {
		return FSERR_BadParam;
	}

	FWObjDesc od;
	int res = findObjectByPath(path, od);
	if(res >= 0) {
		file_t handle = allocateFileDescriptor(od);
		if(handle < 0) {
			res = handle;
			closeObject(od);
		} else {
			*dir = reinterpret_cast<filedir_t>(handle);
		}
	}

	return res;
}

/*
 * TODO: To implement this we could do with storing the directory offset in `stat`.
 */
int FirmwareFileSystem::fopendir(const FileStat* stat, filedir_t* dir)
{
	return FSERR_NotImplemented;
/*

	CHECK_MOUNTED();
	if(dir == nullptr) {
		return FSERR_BadParam;
	}

	FWObjDesc od;
	int res = findObject
	int res = findObjectByPath(path, od);
	if(res >= 0) {
		file_t handle = allocateFileDescriptor(od);
		if(handle < 0) {
			res = handle;
			closeObject(od);
		} else {
			*dir = reinterpret_cast<filedir_t>(handle);
		}
	}

	return res;
*/
}

/* Reading a directory simply gets details about any child named objects.
 * The XXXdir() methods will work on any named objects, including files, although
 * these don't normally contain named children.
 */
int FirmwareFileSystem::readdir(filedir_t dir, FileStat* stat)
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

	FWObjRef ref;
	ref.offset = fd.cursor;
	FWObjDesc od(ref);
	while((res = readChildObjectHeader(odDir, od)) >= 0) {
		if(od.obj.isNamed()) {
			if(stat) {
				FWObjDesc child;
				res = openChildObject(odDir, od, child);
				if(res >= 0) {
					res = fillStat(*stat, child);
					closeObject(child);
				}
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

	return res == FSERR_EndOfObjects ? FSERR_NoMoreFiles : res;
}

int FirmwareFileSystem::closedir(filedir_t dir)
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
int FirmwareFileSystem::findObjectByPath(const char* path, FWObjDesc& od)
{
#ifdef DEBUG_FWFS
	ElapseTimer timer;
#endif

	// Start with the root directory object
	int res = openRootObject(_volumes[0], od);
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

file_t FirmwareFileSystem::fopen(const FileStat& stat, FileOpenFlags flags)
{
	CHECK_MOUNTED();
	if(stat.fs != this) {
		return FSERR_BadParam;
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

file_t FirmwareFileSystem::open(const char* path, FileOpenFlags flags)
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

int FirmwareFileSystem::close(file_t file)
{
	GET_FD();

	debug_d("descriptor #%u close, refCount = %u", file - FWFS_HANDLE_MIN, fd.odFile.ref.refCount);
	assert(fd.odFile.ref.refCount == 1);

	closeObject(fd.odFile);
	fd.attr = 0;
	return FS_OK;
}

int FirmwareFileSystem::stat(const char* path, FileStat* stat)
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

int FirmwareFileSystem::fstat(file_t file, FileStat* stat)
{
	GET_FD();

	return stat ? fillStat(*stat, fd.odFile) : FSERR_BadParam;
}

int FirmwareFileSystem::eof(file_t file)
{
	GET_FD();

	// 0 - not EOF, > 0 - at EOF, < 0 - error
	return fd.cursor >= fd.dataSize ? 1 : 0;
}

int32_t FirmwareFileSystem::tell(file_t file)
{
	GET_FD();

	return fd.cursor;
}

int FirmwareFileSystem::isfile(file_t file)
{
	GET_FD();

	return FS_OK;
}

/*
 * fileid identifies the object store and object identifier, but it doesn't implicitly tell
 * us the path. Whilst we can have aliases and such, we're interested in the 'true' path of the
 * file object. So, we start at the root and iterate through every single object on the system
 * searching for the file. Not pretty, but then the only reason this method is provided is
 * for the hybrid filesystem, which needs to match full file paths across filesystems.
 */
int FirmwareFileSystem::getFilePath(fileid_t fileid, NameBuffer& path)
{
	int res = FS_OK;

	auto& vol = _volumes[0];

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
		res = FSERR_BufferTooSmall;
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
int FirmwareFileSystem::seekFilePath(FWObjDesc& parent, fileid_t fileid, NameBuffer& path)
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

	return res == FSERR_EndOfObjects ? FSERR_NotFound : res;
}
