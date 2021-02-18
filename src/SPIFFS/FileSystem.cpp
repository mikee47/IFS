/*
 * FileSystem.cpp
 *
 *  Created on: 21 Jul 2018
 *      Author: mikee47
 */

#include "../include/IFS/SPIFFS/FileSystem.h"
#include "../include/IFS/SPIFFS/Error.h"
#include "../include/IFS/Util.h"

namespace IFS
{
/** @brief SPIFFS directory object
 *
 */
struct FileDir {
	char path[SPIFFS_OBJ_NAME_LEN]; ///< Filter for readdir()
	unsigned pathlen;
	spiffs_DIR d;
};

namespace SPIFFS
{
constexpr uint32_t logicalBlockSize{4096 * 2};

namespace
{
/** @brief map IFS File::OpenFlags to SPIFFS equivalents
 *  @param flags
 *  @param sflags OUT the SPIFFS file open flags
 *  @retval File::OpenFlags if non-zero then some flags weren't recognised
 *  @note File::OpenFlags were initially created the same as SPIFFS, but they
 * 	may change to support other features so map them here
 */
File::OpenFlags mapFileOpenFlags(File::OpenFlags flags, spiffs_flags& sflags)
{
	sflags = 0;

	auto map = [&](File::OpenFlag flag, spiffs_flags sflag) {
		if(flags[flag]) {
			sflags |= sflag;
			flags -= flag;
		}
	};

	map(File::OpenFlag::Append, SPIFFS_O_APPEND);
	map(File::OpenFlag::Truncate, SPIFFS_O_TRUNC);
	map(File::OpenFlag::Create, SPIFFS_O_CREAT);
	map(File::OpenFlag::Read, SPIFFS_O_RDONLY);
	map(File::OpenFlag::Write, SPIFFS_O_WRONLY);

	if(flags.any()) {
		debug_w("Unknown File::OpenFlags: 0x%02X", flags.value());
	}

	return flags;
}

s32_t f_read(struct spiffs_t* fs, u32_t addr, u32_t size, u8_t* dst)
{
	auto part = static_cast<Storage::Partition*>(fs->user_data);
	return part && part->read(addr, dst, size) ? SPIFFS_OK : SPIFFS_ERR_INTERNAL;
}

s32_t f_write(struct spiffs_t* fs, u32_t addr, u32_t size, u8_t* src)
{
	auto part = static_cast<Storage::Partition*>(fs->user_data);
	return part && part->write(addr, src, size) ? SPIFFS_OK : SPIFFS_ERR_INTERNAL;
}

s32_t f_erase(struct spiffs_t* fs, u32_t addr, u32_t size)
{
	auto part = static_cast<Storage::Partition*>(fs->user_data);
	return part && part->erase_range(addr, size) ? SPIFFS_OK : SPIFFS_ERR_INTERNAL;
}

} // namespace

FileSystem::~FileSystem()
{
	SPIFFS_unmount(handle());
}

int FileSystem::mount()
{
	if(!partition) {
		return Error::NoMedia;
	}

	fs.user_data = &partition;
	spiffs_config cfg{
		.hal_read_f = f_read,
		.hal_write_f = f_write,
		.hal_erase_f = f_erase,
		.phys_size = partition.size(),
		.phys_addr = 0,
		.phys_erase_block = partition.getBlockSize(),
		.log_block_size = logicalBlockSize,
		.log_page_size = LOG_PAGE_SIZE,
	};

	//  debug_i("FFS offset: 0x%08x, size: %u Kb, \n", cfg.phys_addr, cfg.phys_size / 1024);

	auto res = tryMount(cfg);
	if(res < 0) {
		/*
		 * Mount failed, so we either try to repair the system or format it.
		 * For now, just format it.
		 */
		res = format();
	}

#if SPIFFS_TEST_VISUALISATION
	if(res == SPIFFS_OK) {
		SPIFFS_vis(handle());
	}
#endif

	return res;
}

int FileSystem::tryMount(spiffs_config& cfg)
{
	auto err = SPIFFS_mount(handle(), &cfg, reinterpret_cast<uint8_t*>(workBuffer),
							reinterpret_cast<uint8_t*>(fileDescriptors), sizeof(fileDescriptors), cache, sizeof(cache),
							nullptr);
	if(err < 0) {
		if(isSpiffsError(err)) {
			err = Error::fromSystem(err);
		}
		debug_ifserr(err, "SPIFFS_mount()");
	}

	return err;
}

/*
 * Format the file system and leave it mounted in an accessible state.
 */
int FileSystem::format()
{
	spiffs_config cfg = fs.cfg;
	// Must be unmounted before format is called - see API
	SPIFFS_unmount(handle());
	int err = SPIFFS_format(handle());
	if(err < 0) {
		err = Error::fromSystem(err);
		debug_ifserr(err, "format()");
		return err;
	}

	// Re-mount
	return tryMount(cfg);
}

int FileSystem::check()
{
	fs.check_cb_f = [](spiffs* fs, spiffs_check_type type, spiffs_check_report report, u32_t arg1, u32_t arg2) {
		if(report > SPIFFS_CHECK_PROGRESS) {
			debug_d("SPIFFS check %d, %d, %u, %u", type, report, arg1, arg2);
		}
	};

	int err = SPIFFS_check(handle());
	return Error::fromSystem(err);
}

int FileSystem::getinfo(Info& info)
{
	info.clear();
	info.partition = partition;
	info.type = Type::SPIFFS;
	info.maxNameLength = SPIFFS_OBJ_NAME_LEN - 1;
	info.maxPathLength = info.maxNameLength;
	if(SPIFFS_mounted(handle())) {
		info.volumeID = fs.config_magic;
		info.attr |= Attribute::Mounted;
#ifndef SPIFFS_STORE_META
		info.attr |= Attribute::NoMeta;
#endif
		u32_t total, used;
		SPIFFS_info(handle(), &total, &used);
		info.volumeSize = total;
		// As per SPIFFS spec
		if(used >= total) {
			info.attr |= Attribute::Check;
		} else {
			info.freeSpace = total - used;
		}
	}

	return FS_OK;
}

String FileSystem::getErrorString(int err)
{
	if(Error::isSystem(err)) {
		return spiffsErrorToStr(Error::toSystem(err));
	} else {
		return IFS::Error::toString(err);
	}
}

static spiffs_file SPIFFS_open_by_id(spiffs* fs, spiffs_obj_id obj_id, spiffs_flags flags, spiffs_mode mode)
{
	spiffs_fd* fd;
	s32_t res = spiffs_fd_find_new(fs, &fd, 0);
	SPIFFS_CHECK_RES(res);

	res = spiffs_object_open_by_id(fs, obj_id, fd, flags, mode);
	if(res < SPIFFS_OK) {
		spiffs_fd_return(fs, fd->file_nbr);
	}

	SPIFFS_CHECK_RES(res);

#if !SPIFFS_READ_ONLY
	if(flags & SPIFFS_O_TRUNC) {
		res = spiffs_object_truncate(fd, 0, 0);
		if(res < SPIFFS_OK) {
			spiffs_fd_return(fs, fd->file_nbr);
		}

		SPIFFS_CHECK_RES(res);
	}
#endif

	fd->fdoffset = 0;

	return SPIFFS_FH_OFFS(fs, fd->file_nbr);
}

File::Handle FileSystem::fopen(const FileStat& stat, File::OpenFlags flags)
{
	/*
	 * If file is marked read-only, fail write requests.
	 * Note that we trust the FileStat information provided. This provides
	 * a mechanism for an application to circumvent this flag if it becomes
	 * necessary to change  a file.
	 */
	if(stat.attr[File::Attribute::ReadOnly]) {
		return Error::ReadOnly;
	}

	spiffs_flags sflags;
	if(mapFileOpenFlags(flags, sflags).any()) {
		return File::Handle(Error::NotSupported);
	}

	auto file = SPIFFS_open_by_id(handle(), stat.id, sflags, 0);
	if(file < 0) {
		int err = Error::fromSystem(file);
		debug_ifserr(err, "fopen('%s')", stat.name.buffer);
		return err;
	}

	initMetaBuffer(file);
	// File affected without write so update timestamp
	if(flags[File::OpenFlag::Truncate]) {
		touch(file);
	}

	return file;
}

File::Handle FileSystem::open(const char* path, File::OpenFlags flags)
{
	FS_CHECK_PATH(path);
	if(path == nullptr) {
		return Error::BadParam;
	}

	/*
	 * The file may be marked 'read-only' in its metadata, so avoid modifications
	 * (truncate) during the open phase until this has been checked.
	 */

	spiffs_flags sflags;
	if(mapFileOpenFlags(flags - File::OpenFlag::Truncate, sflags).any()) {
		return File::Handle(Error::NotSupported);
	}

	auto file = SPIFFS_open(handle(), path, sflags, 0);
	if(file < 0) {
		int err = Error::fromSystem(file);
		debug_ifserr(err, "open('%s')", path);
		return err;
	}

	auto smb = initMetaBuffer(file);
	// If file is marked read-only, fail write requests
	if(smb != nullptr) {
		if(smb->meta.attr[File::Attribute::ReadOnly] && (flags[File::OpenFlag::Write])) {
			SPIFFS_close(handle(), file);
			return Error::ReadOnly;
		}
	}

	// Now truncate the file if so requested
	if(flags[File::OpenFlag::Truncate]) {
		int err = SPIFFS_ftruncate(handle(), file, 0);
		if(err < 0) {
			SPIFFS_close(handle(), file);
			return Error::fromSystem(err);
		}

		// Update modification timestamp
		touch(file);
	}

	return file;
}

int FileSystem::close(File::Handle file)
{
	if(file < 0) {
		return Error::FileNotOpen;
	}

	int res = flushMeta(file);
	int err = SPIFFS_close(handle(), file);
	if(err < 0) {
		res = Error::fromSystem(err);
	}
	return res;
}

int FileSystem::eof(File::Handle file)
{
	int res = SPIFFS_eof(handle(), file);
	return Error::fromSystem(res);
}

int32_t FileSystem::tell(File::Handle file)
{
	int res = SPIFFS_tell(handle(), file);
	return Error::fromSystem(res);
}

int FileSystem::truncate(File::Handle file, size_t new_size)
{
	int res = SPIFFS_ftruncate(handle(), file, new_size);
	return Error::fromSystem(res);
}

int FileSystem::flush(File::Handle file)
{
	int res = flushMeta(file);
	int err = SPIFFS_fflush(handle(), file);
	if(err < 0) {
		res = Error::fromSystem(err);
	}
	return res;
}

int FileSystem::read(File::Handle file, void* data, size_t size)
{
	int res = SPIFFS_read(handle(), file, data, size);
	if(res < 0) {
		int err = Error::fromSystem(res);
		debug_ifserr(err, "read()");
		return err;
	}

	//  debug_i("read(%d): %d", bufSize, res);
	return res;
}

int FileSystem::write(File::Handle file, const void* data, size_t size)
{
	int res = SPIFFS_write(handle(), file, const_cast<void*>(data), size);
	if(res < 0) {
		return Error::fromSystem(res);
	}

	touch(file);
	return res;
}

int FileSystem::lseek(File::Handle file, int offset, SeekOrigin origin)
{
	int res = SPIFFS_lseek(handle(), file, offset, int(origin));
	if(res < 0) {
		int err = Error::fromSystem(res);
		debug_ifserr(err, "lseek()");
		return err;
	}

	//  debug_i("Seek(%d, %d): %d", offset, origin, res);

	return res;
}

SpiffsMetaBuffer* FileSystem::initMetaBuffer(File::Handle file)
{
	auto smb = getMetaBuffer(file);
	if(smb == nullptr) {
		return nullptr;
	}

	smb->init();

#ifdef SPIFFS_STORE_META
	spiffs_stat stat;
	int err = SPIFFS_fstat(handle(), file, &stat);
	if(err >= 0) {
		smb->assign(stat.meta);
	}
#endif

	return smb;
}

SpiffsMetaBuffer* FileSystem::getMetaBuffer(File::Handle file)
{
	unsigned off = SPIFFS_FH_UNOFFS(handle(), file) - 1;
	if(off >= FFS_MAX_FILEDESC) {
		debug_e("getMetaBuffer(%d) - bad file", file);
		return nullptr;
	}
	return &metaCache[off];
}

/*
 * If metadata has changed, flush it to SPIFFS.
 */
int FileSystem::flushMeta(File::Handle file)
{
#ifdef SPIFFS_STORE_META
	auto smb = getMetaBuffer(file);
	if(smb == nullptr) {
		return Error::InvalidHandle;
	}

	// Changed ?
	if(smb->flags[SpiffsMetaBuffer::Flag::dirty]) {
		debug_d("Flushing Metadata to disk");
		smb->flags[SpiffsMetaBuffer::Flag::dirty] = false;
		int err = SPIFFS_fupdate_meta(handle(), file, smb);
		if(err < 0) {
			err = Error::fromSystem(err);
			debug_ifserr(err, "fupdate_meta()");
			return err;
		}
	}
#endif
	return FS_OK;
}

int FileSystem::stat(const char* path, FileStat* stat)
{
	spiffs_stat ss;
	int err = SPIFFS_stat(handle(), path, &ss);
	if(err < 0) {
		return Error::fromSystem(err);
	}

	if(stat != nullptr) {
		stat->clear();
		stat->fs = this;
		stat->name.copy(reinterpret_cast<const char*>(ss.name));
		stat->size = ss.size;
		stat->id = ss.obj_id;
		SpiffsMetaBuffer smb;
#ifdef SPIFFS_STORE_META
		smb.assign(ss.meta);
#else
		smb.init();
#endif
		smb.copyTo(*stat);
	}

	return FS_OK;
}

int FileSystem::fstat(File::Handle file, FileStat* stat)
{
	spiffs_stat ss;
	int err = SPIFFS_fstat(handle(), file, &ss);
	if(err < 0) {
		return Error::fromSystem(err);
	}

	auto smb = getMetaBuffer(file);
	if(smb == nullptr) {
		return Error::InvalidHandle;
	}

#ifdef SPIFFS_STORE_META
	smb->assign(ss.meta);
#endif

	if(stat != nullptr) {
		stat->clear();
		stat->fs = this;
		stat->name.copy(reinterpret_cast<const char*>(ss.name));
		stat->size = ss.size;
		stat->id = ss.obj_id;
		smb->copyTo(*stat);
	}

	return FS_OK;
}

int FileSystem::setacl(File::Handle file, const File::ACL& acl)
{
	auto smb = getMetaBuffer(file);
	if(smb == nullptr) {
		return Error::InvalidHandle;
	}

	smb->setAcl(acl);
	return FS_OK;
}

int FileSystem::setattr(const char* path, File::Attributes attr)
{
	FS_CHECK_PATH(path);
	if(path == nullptr) {
		return Error::BadParam;
	}

	auto file = SPIFFS_open(handle(), path, SPIFFS_O_RDWR, 0);
	if(file < 0) {
		int err = Error::fromSystem(file);
		debug_ifserr(err, "open('%s')", path);
		return err;
	}

	auto smb = initMetaBuffer(file);
	if(smb == nullptr) {
		SPIFFS_close(handle(), file);
		return Error::InvalidHandle;
	}

	constexpr File::Attributes mask{File::Attribute::ReadOnly | File::Attribute::Archive};
	smb->setattr(smb->meta.attr - mask + (attr & mask));

	return close(file);
}

int FileSystem::settime(File::Handle file, time_t mtime)
{
	auto smb = getMetaBuffer(file);
	if(smb == nullptr) {
		return Error::InvalidHandle;
	}

	smb->setFileTime(mtime);
	return FS_OK;
}

int FileSystem::setcompression(File::Handle file, const File::Compression& compression)
{
	auto smb = getMetaBuffer(file);
	if(smb == nullptr) {
		return Error::InvalidHandle;
	}

	smb->setCompression(compression);
	return FS_OK;
}

int FileSystem::opendir(const char* path, DirHandle& dir)
{
	FS_CHECK_PATH(path);
	unsigned pathlen = 0;
	if(path != nullptr) {
		pathlen = strlen(path);
		if(pathlen >= sizeof(FileDir::path)) {
			return Error::NameTooLong;
		}
	}

	auto d = new FileDir;
	if(d == nullptr) {
		return Error::NoMem;
	}

	if(SPIFFS_opendir(handle(), nullptr, &d->d) == nullptr) {
		int err = SPIFFS_errno(handle());
		err = Error::fromSystem(err);
		debug_ifserr(err, "opendir");
		delete d;
		return err;
	}

	if(pathlen != 0) {
		memcpy(d->path, path, pathlen);
	}
	d->path[pathlen] = '\0';
	d->pathlen = pathlen;

	dir = d;
	return FS_OK;
}

int FileSystem::readdir(DirHandle dir, FileStat& stat)
{
	if(dir == nullptr) {
		return Error::BadParam;
	}

	spiffs_dirent e;
	for(;;) {
		if(SPIFFS_readdir(&dir->d, &e) == nullptr) {
			int err = SPIFFS_errno(handle());
			if(err == SPIFFS_VIS_END) {
				return Error::NoMoreFiles;
			}

			return Error::fromSystem(err);
		}

		/* The volume doesn't contain directory objects, so at each level we need
		 * to identify sub-folders and insert a virtual 'directory' object.
		 */

		auto name = (char*)e.name;
		char* lastSep = strrchr(name, '/');

		// For root directory, include only root objects
		if(dir->pathlen == 0 && lastSep != nullptr) {
			//			debug_i("Ignoring '%s' - root only", name);
			continue;
		}

		// For sub-directories, match the parsing path
		auto len = strlen(name);
		if(dir->pathlen != 0) {
			if(len <= dir->pathlen) {
				//				debug_i("Ignoring '%s' - too short for '%s'", name, dir->path);
				continue;
			}

			if(name[dir->pathlen] != '/') {
				//				debug_i("Ignoring '%s' - no '/' in expected position to match '%s'", name, dir->path);
				continue;
			}

			if(memcmp(dir->path, name, dir->pathlen) != 0) {
				//				debug_i("Ignoring '%s' - doesn't match '%s'", name, dir->path);
				continue;
			}

			name += dir->pathlen + 1;
		}

		// This is a child directory
		char* nextSep = strchr(name, '/');
		if(nextSep != nullptr) {
			*nextSep = '\0';
		}

		stat.clear();
		stat.fs = this;
		stat.name.copy(name);
		if(nextSep != nullptr) {
			stat.attr |= File::Attribute::Directory;
		} else {
			stat.size = e.size;
			stat.id = e.obj_id;
			SpiffsMetaBuffer smb;
#ifdef SPIFFS_STORE_META
			smb.assign(e.meta);
#else
			smb.init();
#endif
			smb.copyTo(stat);
		}

		return FS_OK;
	}
}

int FileSystem::closedir(DirHandle dir)
{
	if(dir == nullptr) {
		return Error::BadParam;
	}

	int err = SPIFFS_closedir(&dir->d);
	delete dir;
	return Error::fromSystem(err);
}

int FileSystem::mkdir(const char* path)
{
	// TODO
	return Error::NotImplemented;
}

int FileSystem::rename(const char* oldpath, const char* newpath)
{
	FS_CHECK_PATH(oldpath);
	FS_CHECK_PATH(newpath);
	if(oldpath == nullptr || newpath == nullptr) {
		return Error::BadParam;
	}

	int err = SPIFFS_rename(handle(), oldpath, newpath);
	return Error::fromSystem(err);
}

int FileSystem::remove(const char* path)
{
	FS_CHECK_PATH(path);
	if(path == nullptr) {
		return Error::BadParam;
	}

	// Check file is not marked read-only
	int f = open(path, File::OpenFlag::Read);
	if(f >= 0) {
		auto smb = getMetaBuffer(f);
		assert(smb != nullptr);
		auto attr = smb->meta.attr;
		close(f);
		if(attr[File::Attribute::ReadOnly]) {
			return Error::ReadOnly;
		}
	}

	int err = SPIFFS_remove(handle(), path);
	err = Error::fromSystem(err);
	debug_ifserr(err, "remove('%s')", path);
	return err;
}

int FileSystem::fremove(File::Handle file)
{
	// If file is marked read-only, fail request
	auto smb = getMetaBuffer(file);
	if(smb == nullptr) {
		return Error::InvalidHandle;
	}
	if(smb->meta.attr[File::Attribute::ReadOnly]) {
		return Error::ReadOnly;
	}

	int err = SPIFFS_fremove(handle(), file);
	return Error::fromSystem(err);
}

int FileSystem::getFilePath(File::ID fileid, NameBuffer& buffer)
{
	auto fs = handle();
	spiffs_block_ix block_ix;
	int lu_entry;
	int err = spiffs_obj_lu_find_id(fs, 0, 0, fileid | SPIFFS_OBJ_ID_IX_FLAG, &block_ix, &lu_entry);
	if(err == SPIFFS_OK) {
		spiffs_page_object_ix_header objix_hdr;
		spiffs_page_ix pix = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(fs, block_ix, lu_entry);
		err = _spiffs_rd(fs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, SPIFFS_PAGE_TO_PADDR(fs, pix),
						 sizeof(objix_hdr), reinterpret_cast<u8_t*>(&objix_hdr));
		if(err == SPIFFS_OK) {
			err = buffer.copy(reinterpret_cast<const char*>(objix_hdr.name));
		}
	}

	return Error::fromSystem(err);
}

} // namespace SPIFFS
} // namespace IFS
