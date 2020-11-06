/*
 * FileSystem.cpp
 *
 *  Created on: 22 Jul 2018
 *      Author: mikee47
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

#define GET_FS(__file)                                                                                                 \
	if(__file < 0) {                                                                                                   \
		return __file;                                                                                                 \
	}                                                                                                                  \
	IFileSystem* fs;                                                                                                   \
	if(fwfs.isfile(file) == FS_OK) {                                                                                   \
		fs = &fwfs;                                                                                                    \
	} else {                                                                                                           \
		fs = &ffs;                                                                                                     \
	}

namespace IFS
{
// opendir() uses this structure to track file listing
struct FileDir {
	// Directory objects for both filing systems
	DirHandle ffs;
	DirHandle fw;
	// The directory object being enumerated
	IFileSystem* fs;
};

namespace HYFS
{
int FileSystem::mount()
{
	// Mount both filesystems so they take ownership of the media objects
	int res = fwfs.mount();
	if(res >= 0) {
		res = ffs.mount();
	}
	return res;
}

int FileSystem::getinfo(Info& info)
{
	Info ffsinfo;
	ffsinfo.name = info.name;
	ffs.getinfo(ffsinfo);
	Info fwinfo;
	if(info.name.length == 0) {
		fwinfo.name = info.name;
	}
	fwfs.getinfo(fwinfo);

	info.type = Type::Hybrid;
	info.attr = fwinfo.attr | ffsinfo.attr;
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
	String s = ffs.getErrorString(err);
	if(!s) {
		s = fwfs.getErrorString(err);
	}
	return s;
}

int FileSystem::hideFWFile(const char* path, bool hide)
{
	int res = FS_OK;
#if HYFS_HIDE_FLAGS == 1
	FileStat stat;
	res = fwfs.stat(path, &stat);
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

bool FileSystem::isFWFileHidden(const FileStat& fwstat)
{
#if HYFS_HIDE_FLAGS == 1
	return hiddenFwFiles.contains(fwstat.id);
#else
	return ffs.stat(fwstat.name, nullptr) >= 0;
#endif
}

/*
 * We need to 'mask out' FW files which are on FFS. One way to do that is to set
 * a flag for any FW file so it's skipped during enumeration. This flag is set
 * during FFS enumeration.
 */

int FileSystem::opendir(const char* path, DirHandle& dir)
{
	auto d = new FileDir;
	if(d == nullptr) {
		return Error::NoMem;
	}

	// Open directories on both filing systems
	int res = ffs.opendir(path, d->ffs);
	if(res >= 0) {
		res = fwfs.opendir(path, d->fw);
		if(res < 0) {
			ffs.closedir(d->ffs);
		}
	}

	if(res < 0) {
		delete d;
	} else {
		d->fs = &ffs;
		dir = d;
	}

	return res;
}

int FileSystem::readdir(DirHandle dir, FileStat& stat)
{
	if(dir == nullptr) {
		return Error::BadParam;
	}

	int res;

	// FFS ?
	if(dir->fs == &ffs) {
		// Use a temporary stat in case it's not provided
		res = ffs.readdir(dir->ffs, stat);
		if(res >= 0) {
			char buf[SPIFFS_OBJ_NAME_LEN];
			NameBuffer name(buf, sizeof(buf));
			int err = ffs.getFilePath(stat.id, name);
			if(err < 0) {
				debug_e("getFilePath(%u) error %d", stat.id, err);
			} else {
				debug_i("getFilePath(%u) - '%s'", stat.id, buf);
				hideFWFile(name.buffer, true);
			}
			return res;
		}

		// End of FFS files
		dir->fs = &fwfs;
	} else if(dir->fs != &fwfs) {
		return Error::BadParam;
	}

	do {
		res = fwfs.readdir(dir->fw, stat);
		if(res < 0) {
			break;
		}
	} while(isFWFileHidden(stat));

	return res;
}

int FileSystem::rewinddir(DirHandle dir)
{
	if(dir == nullptr) {
		return Error::BadParam;
	}

	if(dir->fs == &ffs) {
		return ffs.rewinddir(dir->ffs);
	}

	if(dir->fs == &fwfs) {
		return fwfs.rewinddir(dir->fw);
	}

	return Error::BadParam;
}

int FileSystem::closedir(DirHandle dir)
{
	if(dir == nullptr) {
		return Error::BadParam;
	}

	fwfs.closedir(dir->fw);
	ffs.closedir(dir->ffs);
	delete dir;

	return FS_OK;
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
File::Handle FileSystem::open(const char* path, File::OpenFlags flags)
{
	// If file exists on FFS then open it and return
	FileStat stat;
	int res = ffs.stat(path, &stat);
	if(res >= 0) {
		return ffs.fopen(stat, flags);
	}

	// OK, so no FFS file exists. Get the FW file.
	File::Handle fwfile = fwfs.open(path, File::OpenFlag::Read);

	// If we're only reading the file then return FW file directly
	if(flags == File::OpenFlag::Read) {
		return fwfile;
	}

	// If we have a FW file, check the ReadOnly flag
	if(fwfile >= 0) {
		int err = fwfs.fstat(fwfile, &stat);
		if(err >= 0 && stat.attr[File::Attribute::ReadOnly]) {
			err = Error::ReadOnly;
		}
		if(err < 0) {
			fwfs.close(fwfile);
			return err;
		}
	}

	// Now copy FW file to FFS
	if(fwfile >= 0) {
		flags |= File::OpenFlag::Create | File::OpenFlag::Read | File::OpenFlag::Write;
	}
	File::Handle ffsfile = ffs.open(path, flags);

	// If there's no FW file, nothing further to do so return FFS result (success or failure)
	if(fwfile < 0) {
		return ffsfile;
	}

	// If FFS file creation failed then fail
	if(ffsfile < 0) {
		fwfs.close(fwfile);
		return ffsfile;
	}

	// Copy metadata
	if(fwfs.fstat(fwfile, &stat) >= 0) {
		ffs.setacl(ffsfile, stat.acl);
		ffs.setattr(ffsfile, stat.attr);
		ffs.settime(ffsfile, stat.mtime);
	}

	// If not truncating then copy content into FFS file
	if(!flags[File::OpenFlag::Truncate]) {
		ffs.lseek(ffsfile, 0, SeekOrigin::Start);
		uint8_t buffer[512];
		while(fwfs.eof(fwfile) == 0) {
			int len = fwfs.read(fwfile, buffer, sizeof(buffer));
			//      debug_i("m_fw.read: %d", len);
			if(len <= 0) {
				break;
			}
			len = ffs.write(ffsfile, buffer, len);
			//      debug_i("m_ffs.write: %d", len);
			if(len < 0) {
				ffs.fremove(ffsfile);
				ffs.close(ffsfile);
				fwfs.close(fwfile);
				return len;
			}
		}
		// Move back to beginning if we're not appending
		if(!flags[File::OpenFlag::Append]) {
			ffs.lseek(ffsfile, 0, SeekOrigin::Start);
		}
	}

	fwfs.close(fwfile);

	return ffsfile;
}

/*
 * Problem: stat refers to a file in a sub-directory, and readdir() by design
 * only returns the file name, omitting the path. So we need to do a lower-level
 * SPIFS stat to get the real file path.
 */
File::Handle FileSystem::fopen(const FileStat& stat, File::OpenFlags flags)
{
	if(stat.fs == nullptr) {
		return Error::BadParam;
	}

	if(stat.fs == &ffs) {
		return ffs.fopen(stat, flags);
	}

	// If we're only reading the file then return FW file directly
	if(flags == File::OpenFlag::Read) {
		return fwfs.fopen(stat, flags);
	}

	// Otherwise it'll involve some work...
	char buf[SPIFFS_OBJ_NAME_LEN];
	NameBuffer name(buf, sizeof(buf));
	int res = fwfs.getFilePath(stat.id, name);
	return res < 0 ? res : open(name.buffer, flags);
}

int FileSystem::close(File::Handle file)
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
	int res = ffs.remove(path);
	if(hideFWFile(path, false) == FS_OK) {
		if(res == SPIFFS_ERR_NOT_FOUND) {
			res = Error::ReadOnly;
		}
	}
	return res;
}

int FileSystem::format()
{
#if HYFS_HIDE_FLAGS == 1
	hiddenFwFiles.removeAllElements();
#endif

	return ffs.format();
}

int FileSystem::check()
{
	return ffs.check();
}

int FileSystem::stat(const char* path, FileStat* stat)
{
	int res = ffs.stat(path, stat);
	if(res < 0) {
		res = fwfs.stat(path, stat);
	}
	return res;
}

int FileSystem::fstat(File::Handle file, FileStat* stat)
{
	GET_FS(file)
	return fs->fstat(file, stat);
}

int FileSystem::setacl(File::Handle file, const File::ACL& acl)
{
	GET_FS(file)
	return fs->setacl(file, acl);
}

int FileSystem::setattr(File::Handle file, File::Attributes attr)
{
	GET_FS(file)
	return fs->setattr(file, attr);
}

int FileSystem::settime(File::Handle file, time_t mtime)
{
	GET_FS(file)
	return fs->settime(file, mtime);
}

int FileSystem::read(File::Handle file, void* data, size_t size)
{
	GET_FS(file)
	return fs->read(file, data, size);
}

int FileSystem::write(File::Handle file, const void* data, size_t size)
{
	GET_FS(file)
	return fs->write(file, data, size);
}

int FileSystem::lseek(File::Handle file, int offset, SeekOrigin origin)
{
	GET_FS(file)
	int res = fs->lseek(file, offset, origin);
	//  debug_i("CHybridFileSystem::lseek(%d, %d, %d): %d", file, offset, origin, res);
	return res;
}

int FileSystem::eof(File::Handle file)
{
	GET_FS(file)
	return fs->eof(file);
}

int32_t FileSystem::tell(File::Handle file)
{
	GET_FS(file)
	return fs->tell(file);
}

int FileSystem::truncate(File::Handle file, size_t new_size)
{
	GET_FS(file)
	return fs->truncate(file, new_size);
}

int FileSystem::flush(File::Handle file)
{
	GET_FS(file)
	return fs->flush(file);
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
	auto file = open(oldpath, File::OpenFlag::Read | File::OpenFlag::Write);
	if(file < 0) {
		return file;
	}

	// Close the file and rename it
	close(file);
	return ffs.rename(oldpath, newpath);
}

/*
 * @note can only delete files with write permission, which means the
 * file is on FFS.
 */
int FileSystem::fremove(File::Handle file)
{
	GET_FS(file)
	return fs->fremove(file);
}

} // namespace HYFS
} // namespace IFS
