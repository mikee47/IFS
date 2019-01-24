/*
 * HybridFileSystem.cpp
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

#include "HybridFileSystem.h"

#define GET_FS(__file)                                                                                                 \
	if(__file < 0)                                                                                                     \
		return __file;                                                                                                 \
	IFileSystem* fs;                                                                                                   \
	if(_fw.isfile(file) == FS_OK)                                                                                      \
		fs = &_fw;                                                                                                     \
	else                                                                                                               \
		fs = &_ffs;

// opendir() uses this structure to track file listing
struct FileDir {
	// Directory objects for both filing systems
	filedir_t ffs;
	filedir_t fw;
	// The directory object being enumerated
	IFileSystem* fs;
};

/* CHybridFileSystem */

int HybridFileSystem::mount()
{
	// Mount both filesystems so they take ownership of the media objects
	int res = _fw.mount();
	if(res >= 0)
		res = _ffs.mount();
	return res;
}

int HybridFileSystem::getinfo(FileSystemInfo& info)
{
	FileSystemInfo ffsinfo;
	ffsinfo.name = info.name;
	_ffs.getinfo(ffsinfo);
	FileSystemInfo fwinfo;
	if(info.name.length == 0)
		fwinfo.name = info.name;
	_fw.getinfo(fwinfo);

	info.type = FileSystemType::Hybrid;
	info.attr = fwinfo.attr | ffsinfo.attr;
	bitSet(info.attr, FileSystemAttr::Virtual);
	info.volumeSize = fwinfo.volumeSize + ffsinfo.volumeSize;
	info.freeSpace = ffsinfo.freeSpace;

	return FS_OK;
}

/*
 * With the available information we cannot know which filing system created this
 * error. This relies on the fact that error codes from these two filing systems
 * do not overlap.
 */
int HybridFileSystem::geterrortext(int err, char* buffer, size_t size)
{
	int ret = _ffs.geterrortext(err, buffer, size);
	if(ret < 0)
		ret = _fw.geterrortext(err, buffer, size);
	return ret;
}

int HybridFileSystem::_hideFWFile(const char* path, bool hide)
{
	int res = FS_OK;
#ifdef HFS_HIDE_FLAGS
	FileStat stat;
	res = _fw.stat(path, &stat);
	if(res >= 0) {
		if(hide) {
			if(!m_hiddenFWFiles.contains(stat.id))
				m_hiddenFWFiles.add(stat.id);
		} else
			m_hiddenFWFiles.removeElement(stat.id);
	}
#endif
	return res;
}

bool HybridFileSystem::_isFWFileHidden(const FileStat& fwstat)
{
#ifdef HFS_HIDE_FLAGS
	return m_hiddenFWFiles.contains(fwstat.id);
#else
	return _ffs.stat(fwstat.name, nullptr) >= 0;
#endif
}

/*
 * We need to 'mask out' FW files which are on FFS. One way to do that is to set
 * a flag for any FW file so it's skipped during enumeration. This flag is set
 * during FFS enumeration.
 */

int HybridFileSystem::opendir(const char* path, filedir_t* dir)
{
	auto d = new FileDir;
	if(!d)
		return FSERR_NoMem;

	// Open directories on both filing systems
	int res = _ffs.opendir(path, &d->ffs);
	if(res >= 0) {
		res = _fw.opendir(path, &d->fw);
		if(res < 0)
			_ffs.closedir(d->ffs);
	}

	if(res < 0)
		delete d;
	else {
		d->fs = &_ffs;
		*dir = d;
	}

	return res;
}

int HybridFileSystem::readdir(filedir_t dir, FileStat* stat)
{
	if(!dir)
		return FSERR_BadParam;

	int res;

	// FFS ?
	if(dir->fs == &_ffs) {
		// Use a temporary stat in case it's not provided
		FileStat tmp;
		if(stat)
			tmp.name = stat->name;
		res = _ffs.readdir(dir->ffs, &tmp);
		if(res >= 0) {
			char buf[SPIFFS_OBJ_NAME_LEN];
			NameBuffer name(buf, sizeof(buf));
			int err = _ffs.getFilePath(tmp.id, name);
			if(err < 0)
				debug_e("getFilePath(%u) error %d", tmp.id, err);
			else {
				debug_i("getFilePath(%u) - '%s'", tmp.id, buf);
				_hideFWFile(name, true);
			}
			if(stat)
				*stat = tmp;
			return res;
		}

		// End of FFS files
		dir->fs = &_fw;
	} else if(dir->fs != &_fw)
		return FSERR_BadParam;

	do {
		res = _fw.readdir(dir->fw, stat);
		if(res < 0)
			break;
	} while(_isFWFileHidden(*stat));

	return res;
}

int HybridFileSystem::closedir(filedir_t dir)
{
	if(!dir)
		return FSERR_BadParam;

	_fw.closedir(dir->fw);
	_ffs.closedir(dir->ffs);
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
file_t HybridFileSystem::open(const char* path, FileOpenFlags flags)
{
	// If file exists on FFS then open it and return
	FileStat stat;
	int res = _ffs.stat(path, &stat);
	if(res >= 0)
		return _ffs.fopen(stat, flags);

	// OK, so no FFS file exists. Get the FW file.
	file_t fwfile = _fw.open(path, eFO_ReadOnly);

	// If we're only reading the file then return FW file directly
	if((flags & ~eFO_ReadOnly) == 0)
		return fwfile;

	// If we have a FW file, check the ReadOnly flag
	if(fwfile >= 0) {
		int err = _fw.fstat(fwfile, &stat);
		if(err >= 0 && bitRead(stat.attr, FileAttr::ReadOnly))
			err = FSERR_ReadOnly;
		if(err < 0) {
			_fw.close(fwfile);
			return err;
		}
	}

	// Now copy FW file to FFS
	if(fwfile >= 0)
		flags = flags | eFO_CreateIfNotExist | eFO_ReadWrite;
	file_t ffsfile = _ffs.open(path, flags);

	// If there's no FW file, nothing further to do so return FFS result (success or failure)
	if(fwfile < 0)
		return ffsfile;

	// If FFS file creation failed then fail
	if(ffsfile < 0) {
		_fw.close(fwfile);
		return ffsfile;
	}

	// Copy metadata
	if(_fw.fstat(fwfile, &stat) >= 0) {
		_ffs.setacl(ffsfile, &stat.acl);
		_ffs.setattr(ffsfile, stat.attr);
		_ffs.settime(ffsfile, stat.mtime);
	}

	// If not truncating then copy content into FFS file
	if(!(flags & eFO_Truncate)) {
		_ffs.lseek(ffsfile, 0, eSO_FileStart);
		uint8_t buffer[512];
		while(_fw.eof(fwfile) == 0) {
			int len = _fw.read(fwfile, buffer, sizeof(buffer));
			//      debug_i("m_fw.read: %d", len);
			if(len <= 0)
				break;
			len = _ffs.write(ffsfile, buffer, len);
			//      debug_i("m_ffs.write: %d", len);
			if(len < 0) {
				_ffs.fremove(ffsfile);
				_ffs.close(ffsfile);
				_fw.close(fwfile);
				return len;
			}
		}
		// Move back to beginning if we're not appending
		if(!(flags & eFO_Append))
			_ffs.lseek(ffsfile, 0, eSO_FileStart);
	}

	_fw.close(fwfile);

	return ffsfile;
}

/*
 * Problem: stat refers to a file in a sub-directory, and readdir() by design
 * only returns the file name, omitting the path. So we need to do a lower-level
 * SPIFS stat to get the real file path.
 */
file_t HybridFileSystem::fopen(const FileStat& stat, FileOpenFlags flags)
{
	if(!stat.fs)
		return FSERR_BadParam;

	if(stat.fs == &_ffs)
		return _ffs.fopen(stat, flags);

	// If we're only reading the file then return FW file directly
	if((flags & ~eFO_ReadOnly) == 0)
		return _fw.fopen(stat, flags);

	// Otherwise it'll involve some work...
	char buf[SPIFFS_OBJ_NAME_LEN];
	NameBuffer name(buf, sizeof(buf));
	int res = _fw.getFilePath(stat.id, name);
	return res < 0 ? res : open(name, flags);
}

int HybridFileSystem::close(file_t file)
{
	GET_FS(file)
	return fs->close(file);
}

/*
 * @todo if file isn't on ffs, but is on fw, then don't report an error.
 * needs a bit of thought this one; do we hide the FW file or unhide it?
 * perhaps an 'unremove' method...
 */
int HybridFileSystem::remove(const char* path)
{
	int res = _ffs.remove(path);
	if(_hideFWFile(path, false) == FS_OK)
		if(res == SPIFFS_ERR_NOT_FOUND)
			res = FSERR_ReadOnly;
	return res;
}

int HybridFileSystem::format()
{
#ifdef HFS_HIDE_FLAGS
	m_hiddenFWFiles.removeAllElements();
#endif

	return _ffs.format();
}

int HybridFileSystem::check()
{
	return _ffs.check();
}

int HybridFileSystem::stat(const char* path, FileStat* stat)
{
	int res = _ffs.stat(path, stat);
	if(res < 0)
		res = _fw.stat(path, stat);
	return res;
}

int HybridFileSystem::fstat(file_t file, FileStat* stat)
{
	GET_FS(file)
	return fs->fstat(file, stat);
}

int HybridFileSystem::setacl(file_t file, FileACL* acl)
{
	GET_FS(file)
	return fs->setacl(file, acl);
}

int HybridFileSystem::setattr(file_t file, FileAttributes attr)
{
	GET_FS(file)
	return fs->setattr(file, attr);
}

int HybridFileSystem::settime(file_t file, time_t mtime)
{
	GET_FS(file)
	return fs->settime(file, mtime);
}

int HybridFileSystem::read(file_t file, void* data, size_t size)
{
	GET_FS(file)
	return fs->read(file, data, size);
}

int HybridFileSystem::write(file_t file, const void* data, size_t size)
{
	GET_FS(file)
	return fs->write(file, data, size);
}

int HybridFileSystem::lseek(file_t file, int offset, SeekOriginFlags origin)
{
	GET_FS(file)
	int res = fs->lseek(file, offset, origin);
	//  debug_i("CHybridFileSystem::lseek(%d, %d, %d): %d", file, offset, origin, res);
	return res;
}

int HybridFileSystem::eof(file_t file)
{
	GET_FS(file)
	return fs->eof(file);
}

int32_t HybridFileSystem::tell(file_t file)
{
	GET_FS(file)
	return fs->tell(file);
}

int HybridFileSystem::truncate(file_t file)
{
	GET_FS(file)
	return fs->truncate(file);
}

int HybridFileSystem::flush(file_t file)
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
int HybridFileSystem::rename(const char* oldpath, const char* newpath)
{
	// Make sure file exists on FFS
	auto file = open(oldpath, eFO_ReadWrite);
	if(file < 0)
		return file;

	// Close the file and rename it
	close(file);
	return _ffs.rename(oldpath, newpath);
}

/*
 * @note can only delete files with write permission, which means the
 * file is on FFS.
 */
int HybridFileSystem::fremove(file_t file)
{
	GET_FS(file)
	return fs->fremove(file);
}
