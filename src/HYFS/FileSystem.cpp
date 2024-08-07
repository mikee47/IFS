/**
 * FileSystem.cpp
 *
 * Created on: 22 Jul 2018
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

/*
 *
 * Enumerating the file system is probably the trickiest bit.
 * If the files in both systems are sorted this makes life much
 * easier. Otherwise we need to build an in-memory list.
 * Alternatively we just start by enumerating all SPIFFS files,
 * then all firmware files. Less efficient but gets the job done.
 *
 *
 * Masking / hiding firmware files
 * -------------------------------
 *
 * A default system will have all files read directly from FW, with
 * the FFS empty. When a file is modified it's pulled from FW and
 * copied to FFS. We need a method to prevent the FW files from
 * appearing in the file system twice.
 *
 * So what are our operating scenarios?
 *
 *  1. Default network configuration. Stored on FW to enforce
 *     local access only for configuring the device. Replaced
 *     with FFS file. Since we always need this file, a delete
 *     operation could just mess things up unless we also have
 *     hard-coded defaults.
 *  2. Files served to web browser. Updates and modifications
 *     can be deployed to FFS without requiring a firmware update.
 *     Deleting of files seems rather pointless because if the
 *     structure changes significantly we'd be considering a
 *     firmware update also. We could instead just write
 *     zero-length files.
 *
 * A delete operation therefore removes the file from FFS (if it
 * exists) and reveals the original FW file (if any).
 *
 * Enumeration always starts with FFS, followed by FW. When
 * enumerating FW we need to know which files to 'hide' to avoid
 * confusion with FFS. We can either use a bitmap or a linear
 * list of hidden file IDs (file indices).
 * The number of hidden files should be relatively small, but there
 * could be far more files on the system. Let's consider some values:
 *
 *  ___ Counts ___    __ Bytes Used __
 *  Files   Hidden    Bitmap    Array (16)    Array (8)
 *  -----   ------    ------    ----------    ---------
 *  32      5         4         10            5
 *  256     5         32        10            5
 *  256     32        32        64            32
 *  1024    64        128       256           128
 *  2048    256       256       512           256
 *
 * The bitmap is a fixed sized, whereas the array will be variable,
 * depending on how many hidden files there are. If we constrain
 * the system to 256 'overwriteable' files then we only need 8 bits
 * for the index entry; in this scenario we'd fail any write requests
 * to files with (index > 255).
 *
 * The less efficient way to do this would be to directly search FFS for
 * every iteration of FW.
 *
 */

#include "../include/IFS/HYFS/FileSystem.h"
#include "../include/IFS/FWFS/FileSystem.h"
#include "../include/IFS/FileSystem.h"
#include "../include/IFS/Util.h"

#define CHECK_MOUNTED()                                                                                                \
	if(!mounted) {                                                                                                     \
		return Error::NotMounted;                                                                                      \
	}

#define GET_FS(handle)                                                                                                 \
	CHECK_MOUNTED()                                                                                                    \
	if(handle < 0) {                                                                                                   \
		return handle;                                                                                                 \
	}                                                                                                                  \
	IFileSystem* fs;                                                                                                   \
	if(handle >= FWFS_HANDLE_MIN && handle < FWFS_HANDLE_MAX) {                                                        \
		fs = fwfs;                                                                                                     \
	} else {                                                                                                           \
		fs = ffs;                                                                                                      \
	}

namespace IFS::HYFS
{
// opendir() uses this structure to track file listing
struct FileDir {
	CString path;
	// Directory objects for both filing systems
	DirHandle ffs;
	DirHandle fw;
	// The directory object being enumerated
	IFileSystem* fs;
};

int FileSystem::mount()
{
	if(mounted) {
		return FS_OK;
	}

	if(fwfs == nullptr || ffs == nullptr) {
		return Error::NoFileSystem;
	}

	Info info;
	ffs->getinfo(info);
	debug_i("[HYFS] Mounting with %s", toString(info.type).c_str());
	if(info.attr[Attribute::ReadOnly]) {
		debug_e("[HYFS] Provided filesystem is read-only");
		return Error::ReadOnly;
	}

	int res = fwfs->mount();
	if(res < 0) {
		return res;
	}

	res = ffs->mount();
	if(res < 0) {
		return res;
	}

	// Copy default root ACL from fwfs -> ffs
	Stat stat{};
	fwfs->stat(nullptr, &stat);
	auto rootAcl = stat.acl;
	ffs->stat(nullptr, &stat);

	auto checkAce = [&](AttributeTag tag, UserRole src, UserRole dst) {
		if(src != dst) {
			int err = ffs->setxattr(nullptr, tag, &src, sizeof(src));
			(void)err;
			debug_i("[HYFS] Root %s -> %s (%s)", toString(tag).c_str(), toString(src).c_str(),
					ffs->getErrorString(err).c_str());
		}
	};
	checkAce(AttributeTag::ReadAce, rootAcl.readAccess, stat.acl.readAccess);
	checkAce(AttributeTag::WriteAce, rootAcl.writeAccess, stat.acl.writeAccess);

	mounted = true;

	return res;
}

int FileSystem::getinfo(Info& info)
{
	Info ffsinfo;
	ffsinfo.name = info.name;
	ffs->getinfo(ffsinfo);
	Info fwinfo;
	if(info.name.length == 0) {
		fwinfo.name = info.name;
	}
	fwfs->getinfo(fwinfo);

	info.type = Type::Hybrid;
	info.maxNameLength = ffsinfo.maxNameLength;
	info.maxPathLength = ffsinfo.maxPathLength;
	info.attr = (fwinfo.attr | ffsinfo.attr) - Attribute::ReadOnly;
	bitSet(info.attr, Attribute::Virtual);
	info.volumeSize = fwinfo.volumeSize + ffsinfo.volumeSize;
	info.freeSpace = ffsinfo.freeSpace;

	return FS_OK;
}

/*
 * With the available information we cannot know which filing system created this
 * error. This relies on the fact that error codes from these two filing systems
 * do not overlap.
 */
String FileSystem::getErrorString(int err)
{
	if(err < Error::SYSTEM) {
		return ffs->getErrorString(err);
	} else {
		return fwfs->getErrorString(err);
	}
}

int FileSystem::setVolume(uint8_t index, IFileSystem* fileSystem)
{
	CHECK_MOUNTED()
	return fwfs->setVolume(index, fileSystem);
}

int FileSystem::hideFWFile(const char* path, bool hide)
{
	int res = FS_OK;
#if HYFS_HIDE_FLAGS == 1
	Stat stat;
	res = fwfs->stat(path, &stat);
	if(res >= 0) {
		if(hide) {
			if(!hiddenFwFiles.contains(stat.id)) {
				hiddenFwFiles.add(stat.id);
			}
		} else {
			hiddenFwFiles.removeElement(stat.id);
		}
	}
#endif
	return res;
}

bool FileSystem::isFWFileHidden(const Stat& fwstat)
{
#if HYFS_HIDE_FLAGS == 1
	return hiddenFwFiles.contains(fwstat.id);
#else
	return ffs->stat(fwstat.name, nullptr) >= 0;
#endif
}

/*
 * We need to 'mask out' FW files which are on FFS. One way to do that is to set
 * a flag for any FW file so it's skipped during enumeration. This flag is set
 * during FFS enumeration.
 */

int FileSystem::opendir(const char* path, DirHandle& dir)
{
	CHECK_MOUNTED()
	FS_CHECK_PATH(path)

	auto d = new FileDir{};
	if(d == nullptr) {
		return Error::NoMem;
	}

	// Open valid directory on FFS if exists, otherwise on FWFS
	int res = ffs->opendir(path, d->ffs);
	if(res < 0) {
		res = fwfs->opendir(path, d->fw);
		if(res < 0) {
			delete d;
			return res;
		}
		d->fs = fwfs;
	} else {
		d->fs = ffs;
	}

	d->path = path;
	dir = DirHandle(d);
	return FS_OK;
}

int FileSystem::readdir(DirHandle dir, Stat& stat)
{
	GET_FILEDIR()

	int res;

	// FFS ?
	if(d->fs == ffs) {
		// Use a temporary stat in case it's not provided
		NameStat s;
		res = ffs->readdir(d->ffs, s);
		if(res >= 0) {
			stat = s;
			auto pathlen = d->path.length();
			auto newpathlen = pathlen + s.name.length;
			if(pathlen != 0) {
				++newpathlen; // Separator
			}
			if(newpathlen >= s.name.size) {
				return Error::NameTooLong;
			}
			char* path = s.name.buffer;
			if(pathlen != 0) {
				memmove(path + pathlen + 1, s.name.buffer, s.name.length + 1);
				path[pathlen] = '/';
				memcpy(path, d->path.c_str(), pathlen);
			}
			hideFWFile(path, true);
			return res;
		}

		// End of FFS files
		if(d->fw == nullptr) {
			res = fwfs->opendir(d->path.c_str(), d->fw);
			if(res == Error::NotFound) {
				return Error::NoMoreFiles;
			}
			if(res < 0) {
				return res;
			}
		}
		d->fs = fwfs;
	} else if(d->fs != fwfs) {
		return Error::BadParam;
	}

	do {
		res = fwfs->readdir(d->fw, stat);
		if(res < 0) {
			break;
		}
	} while(isFWFileHidden(stat));

	return res;
}

int FileSystem::rewinddir(DirHandle dir)
{
	GET_FILEDIR()

	if(d->fw != nullptr) {
		d->fs = fwfs;
		int res = fwfs->rewinddir(d->fw);
		if(res < 0) {
			return res;
		}
	}

	if(d->ffs == nullptr) {
		return FS_OK;
	}

	d->fs = ffs;
	return ffs->rewinddir(d->ffs);
}

int FileSystem::closedir(DirHandle dir)
{
	GET_FILEDIR()

	fwfs->closedir(d->fw);
	ffs->closedir(d->ffs);
	delete d;

	return FS_OK;
}

int FileSystem::mkdir(const char* path)
{
	// TODO
	return Error::NotImplemented;
}

/*
 * For writing we need to do some work.
 *
 *  If file exists on FFS:
 *    OK
 *  else:
 *    if FFS.open succeeds:
 *      If file exists on FW:
 *        copy metadata to FFS stream
 *        if not truncate:
 *          copy content to FFS stream
 *          if append:
 *            seek to EOF
 *          else
 *            seek to BOF
 *    else:
 *      fail
 *
 */
FileHandle FileSystem::open(const char* path, OpenFlags flags)
{
	CHECK_MOUNTED()

	// If file exists on FFS then open it and return
	int res = ffs->stat(path, nullptr);
	if(res >= 0) {
		return ffs->open(path, flags);
	}

	// OK, so no FFS file exists. Get the FW file.
	FileHandle fwfile = fwfs->open(path, OpenFlag::Read);

	// If we're only reading the file then return FW file directly
	if(flags == OpenFlag::Read) {
		return fwfile;
	}

	// If we have a FW file, check the ReadOnly flag
	if(fwfile >= 0) {
		Stat stat;
		int err = fwfs->fstat(fwfile, &stat);
		if(err >= 0 && stat.attr[FileAttribute::ReadOnly]) {
			err = Error::ReadOnly;
		}
		if(err < 0) {
			fwfs->close(fwfile);
			return err;
		}
	}

	// Now copy FW file to FFS
	IFS::FileSystem::cast(ffs)->makedirs(path);
	if(fwfile >= 0) {
		flags |= OpenFlag::Create | OpenFlag::Read | OpenFlag::Write;
	}
	FileHandle ffsfile = ffs->open(path, flags);

	// If there's no FW file, nothing further to do so return FFS result (success or failure)
	if(fwfile < 0) {
		return ffsfile;
	}

	// If FFS file creation failed then fail
	if(ffsfile < 0) {
		fwfs->close(fwfile);
		return ffsfile;
	}

	// Copy metadata
	auto callback = [&](IFS::AttributeEnum& e) -> bool {
		// Ignore errors here as destination file system doesn't necessarily support all attributes
		int err = ffs->fsetxattr(ffsfile, e.tag, e.buffer, e.size);
		if(err < 0) {
			debug_w("[HYFS] fsetxattr(%s): %s", toString(e.tag).c_str(), ffs->getErrorString(err).c_str());
		}
		return true;
	};
	char buffer[1024];
	res = fwfs->fenumxattr(fwfile, callback, buffer, sizeof(buffer));
	if(res < 0) {
		debug_w("[HYFS] fenumxattr(): %s", fwfs->getErrorString(res).c_str());
	}

	// If not truncating then copy content into FFS file
	if(!flags[OpenFlag::Truncate]) {
		ffs->lseek(ffsfile, 0, SeekOrigin::Start);
		uint8_t buffer[512];
		while(fwfs->eof(fwfile) == 0) {
			int len = fwfs->read(fwfile, buffer, sizeof(buffer));
			//      debug_i("m_fw.read: %d", len);
			if(len <= 0) {
				break;
			}
			len = ffs->write(ffsfile, buffer, len);
			if(len < 0) {
				ffs->fremove(ffsfile);
				ffs->close(ffsfile);
				fwfs->close(fwfile);
				return len;
			}
		}
		// Move back to beginning if we're not appending
		if(!flags[OpenFlag::Append]) {
			ffs->lseek(ffsfile, 0, SeekOrigin::Start);
		}
	}

	fwfs->close(fwfile);

	return ffsfile;
}

int FileSystem::close(FileHandle file)
{
	GET_FS(file)
	return fs->close(file);
}

/*
 * @todo if file isn't on ffs, but is on fw, then don't report an error.
 * needs a bit of thought this one; do we hide the FW file or unhide it?
 * perhaps an 'unremove' method...
 */
int FileSystem::remove(const char* path)
{
	CHECK_MOUNTED()
	if(isRootPath(path)) {
		return Error::BadParam;
	}

	int res = ffs->remove(path);
	if(hideFWFile(path, false) == FS_OK) {
		if(res < 0) {
			res = Error::ReadOnly;
		}
	}
	return res;
}

int FileSystem::format()
{
	if(ffs == nullptr) {
		return Error::NoFileSystem;
	}

#if HYFS_HIDE_FLAGS == 1
	hiddenFwFiles.removeAllElements();
#endif

	return ffs->format();
}

int FileSystem::check()
{
	if(ffs == nullptr) {
		return Error::NoFileSystem;
	}

	return ffs->check();
}

int FileSystem::stat(const char* path, Stat* stat)
{
	CHECK_MOUNTED()

	int res = ffs->stat(path, stat);
	if(res < 0) {
		res = fwfs->stat(path, stat);
	}
	return res;
}

int FileSystem::fstat(FileHandle file, Stat* stat)
{
	GET_FS(file)
	return fs->fstat(file, stat);
}

int FileSystem::fcontrol(FileHandle file, ControlCode code, void* buffer, size_t bufSize)
{
	GET_FS(file)
	return fs->fcontrol(file, code, buffer, bufSize);
}

int FileSystem::fsetxattr(FileHandle file, AttributeTag tag, const void* data, size_t size)
{
	GET_FS(file)
	return fs->fsetxattr(file, tag, data, size);
}

int FileSystem::fgetxattr(FileHandle file, AttributeTag tag, void* buffer, size_t size)
{
	GET_FS(file)
	return fs->fgetxattr(file, tag, buffer, size);
}

int FileSystem::fenumxattr(FileHandle file, AttributeEnumCallback callback, void* buffer, size_t bufsize)
{
	GET_FS(file)
	return fs->fenumxattr(file, callback, buffer, bufsize);
}

int FileSystem::setxattr(const char* path, AttributeTag tag, const void* data, size_t size)
{
	CHECK_MOUNTED()

	int res = ffs->setxattr(path, tag, data, size);
	return (res == Error::NotFound) ? Error::ReadOnly : res;
}

int FileSystem::getxattr(const char* path, AttributeTag tag, void* buffer, size_t size)
{
	CHECK_MOUNTED()

	int res = ffs->getxattr(path, tag, buffer, size);
	if(res < 0) {
		res = fwfs->getxattr(path, tag, buffer, size);
	}
	return res;
}

int FileSystem::read(FileHandle file, void* data, size_t size)
{
	GET_FS(file)
	return fs->read(file, data, size);
}

int FileSystem::write(FileHandle file, const void* data, size_t size)
{
	GET_FS(file)
	return fs->write(file, data, size);
}

file_offset_t FileSystem::lseek(FileHandle file, file_offset_t offset, SeekOrigin origin)
{
	GET_FS(file)
	auto res = fs->lseek(file, offset, origin);
	//  debug_i("CHybridFileSystem::lseek(%d, %d, %d): %d", file, offset, origin, res);
	return res;
}

int FileSystem::eof(FileHandle file)
{
	GET_FS(file)
	return fs->eof(file);
}

file_offset_t FileSystem::tell(FileHandle file)
{
	GET_FS(file)
	return fs->tell(file);
}

int FileSystem::ftruncate(FileHandle file, file_size_t new_size)
{
	GET_FS(file)
	return fs->ftruncate(file, new_size);
}

int FileSystem::flush(FileHandle file)
{
	GET_FS(file)
	return fs->flush(file);
}

int FileSystem::fgetextents(FileHandle file, Storage::Partition* part, Extent* list, uint16_t extcount)
{
	GET_FS(file)
	return fs->fgetextents(file, part, list, extcount);
}

/*
 * OK, so here's another tricky one like open. Easiest way to deal with it:
 *  open file in write mode
 *  close file
 *  rename
 */
int FileSystem::rename(const char* oldpath, const char* newpath)
{
	// Make sure file exists on FFS
	auto file = open(oldpath, OpenFlag::Read | OpenFlag::Write);
	if(file < 0) {
		return file;
	}

	// Close the file and rename it
	close(file);
	return ffs->rename(oldpath, newpath);
}

/*
 * @note can only delete files with write permission, which means the
 * file is on FFS.
 */
int FileSystem::fremove(FileHandle file)
{
	GET_FS(file)
	return fs->fremove(file);
}

} // namespace IFS::HYFS
