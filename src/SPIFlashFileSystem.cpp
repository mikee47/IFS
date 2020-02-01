/*
 * SPIFlashFileSystem.cpp
 *
 *  Created on: 21 Jul 2018
 *      Author: mikee47
 */

#include <IFS/SPIFlashFileSystem.h>
#include <IFS/SpiffsError.h>
extern "C" {
#include <spiffs_nucleus.h>
}
#include <IFS/IFSUtil.h>

/*
 * Number of pages to cache
 */
#define CACHE_PAGES 8

#define WORK_BUF_SIZE(_log_page_size) ((_log_page_size)*2)

#define FDS_BUF_SIZE(_num_filedesc) ((_num_filedesc) * sizeof(spiffs_fd))

#define CACHE_SIZE(_pages) (sizeof(spiffs_cache) + (_pages) * (sizeof(spiffs_cache_page) + cfg.log_page_size))

#define SPIFFS_LOG_PAGE_SIZE 256

#if SPIFFS_FILEHDL_OFFSET
#define SPIFFS_
// an integer offset added to each file handle
u16_t fh_ix_offset;
#endif

/** @brief SPIFFS directory object
 *
 */
struct FileDir {
	char path[SPIFFS_OBJ_NAME_LEN]; ///< Filter for readdir()
	unsigned pathlen;
	spiffs_DIR d;
};

/** @brief map IFS FileOpenFlags to SPIFFS equivalents
 *  @param flags
 *  @param sflags OUT the SPIFFS file open flags
 *  @retval FileOpenFlags if non-zero then some flags weren't recognised
 *  @note FileOpenFlags were initially created the same as SPIFFS, but they
 * 	may change to support other features so map them here
 */
static inline FileOpenFlags mapFileOpenFlags(FileOpenFlags flags, spiffs_flags& sflags)
{
	sflags = 0;

	auto map = [&](FileOpenFlags flag, spiffs_flags sflag) {
		if(flags & flag) {
			sflags |= sflag;
			flags = flags - flag;
		}
	};

	map(eFO_Append, SPIFFS_O_APPEND);
	map(eFO_Truncate, SPIFFS_O_TRUNC);
	map(eFO_CreateIfNotExist, SPIFFS_O_CREAT);
	map(eFO_ReadOnly, SPIFFS_O_RDONLY);
	map(eFO_WriteOnly, eFO_WriteOnly);

	if(flags)
		debug_w("Unknown FileOpenFlags: 0x%02X", flags);

	return flags;
}

/*
 * Check metadata record to ensure it's valid, adjusting if required
 */
static void checkMeta(FileMeta& meta)
{
	// Formatted flash values means metadata is un-initialised, so use default
	if((uint32_t)meta.mtime == 0xFFFFFFFF) {
		meta.attr = 0;
		meta.mtime = fsGetTimeUTC();
		/*
		 * @todo find a way to allow the user to explictly specify default file permissions
		 * I think Linux has a ustat or something which does this.
		 */
		meta.acl.readAccess = UserRole::Admin;
		meta.acl.writeAccess = UserRole::Admin;
	}
}

/*
 * Copy metadata fields into FileStat structure.
 * Meta must have been previously verified; this simply translates the values across.
 */
static void copyMeta(FileStat& stat, const FileMeta& meta)
{
	stat.acl = meta.acl;
	stat.attr = meta.attr;
	stat.mtime = meta.mtime;
}

/* SPIFlashFileSystem */

SPIFlashFileSystem::~SPIFlashFileSystem()
{
	SPIFFS_unmount(handle());
	delete[] cache;
	delete[] fileDescriptors;
	delete[] workBuffer;
	delete media;
}

static inline uint32_t Align(uint32_t value, uint32_t gran)
{
	if(gran) {
		uint32_t rem = value % gran;
		if(rem)
			value += gran - rem;
	}
	return value;
}

#define MIN_BLOCKSIZE 256

static s32_t f_read(struct spiffs_t* fs, u32_t addr, u32_t size, u8_t* dst)
{
	return reinterpret_cast<IFSMedia*>(fs->user_data)->read(addr, size, dst);
}

static s32_t f_write(struct spiffs_t* fs, u32_t addr, u32_t size, u8_t* src)
{
	return reinterpret_cast<IFSMedia*>(fs->user_data)->write(addr, size, src);
}

static s32_t f_erase(struct spiffs_t* fs, u32_t addr, u32_t size)
{
	return reinterpret_cast<IFSMedia*>(fs->user_data)->erase(addr, size);
}

int SPIFlashFileSystem::mount()
{
	if(!media)
		return FSERR_NoMedia;

	uint32_t blockSize = media->blockSize();
	if(blockSize < MIN_BLOCKSIZE)
		blockSize = Align(MIN_BLOCKSIZE, blockSize);

	fs.user_data = media;
	spiffs_config cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.hal_read_f = f_read;
	cfg.hal_write_f = f_write;
	cfg.hal_erase_f = f_erase;
	cfg.phys_size = media->mediaSize();
	cfg.phys_addr = 0; // Media object extents start at 0
	cfg.phys_erase_block = blockSize;
	cfg.log_block_size = blockSize * 2;
	cfg.log_page_size = SPIFFS_LOG_PAGE_SIZE;
#if SPIFFS_FILEHDL_OFFSET
	/* an offset added to each file handle
	 * SPIFFS allocates file handles in the range 1 .. FFS_MAX_FILEDESC
	 * If we specify fh_ix_offset then the range can be determined to allow the owning filesystem
	 * to be determined from its handle.
	 */
	// cfg.fh_ix_offset = ...
#endif

	//  debug_i("FFS offset: 0x%08x, size: %u Kb, \n", cfg.phys_addr, cfg.phys_size / 1024);

	workBuffer = new uint8_t[WORK_BUF_SIZE(cfg.log_page_size)];
	fileDescriptors = new uint8_t[FDS_BUF_SIZE(FFS_MAX_FILEDESC)];
	cache = new uint8_t[CACHE_SIZE(CACHE_PAGES)];

	if(!workBuffer || !fileDescriptors || !cache) {
		return FSERR_NoMem;
	}

	auto res = _mount(cfg);
	if(res < 0) {
		/*
     * Mount failed, so we either try to repair the system or format it.
     * For now, just format it.
     */
		res = format();
	}

#if SPIFFS_TEST_VISUALISATION
	if(res == SPIFFS_OK)
		SPIFFS_vis(handle());
#endif

	return res;
}

int SPIFlashFileSystem::_mount(spiffs_config& cfg)
{
	auto res = SPIFFS_mount(handle(), &cfg, workBuffer, fileDescriptors, FDS_BUF_SIZE(FFS_MAX_FILEDESC), cache,
							CACHE_SIZE(CACHE_PAGES), nullptr);
	if(res < 0)
		debug_ifserr(res, "SPIFFS_mount()");
	return res;
}

/*
 * Format the file system and leave it mounted in an accessible state.
 */
int SPIFlashFileSystem::format()
{
	spiffs_config cfg = fs.cfg;
	// Must be unmounted before format is called - see API
	SPIFFS_unmount(handle());
	int res = SPIFFS_format(handle());
	if(res < 0) {
		debug_ifserr(res, "format()");
		return res;
	}

	// Re-mount
	return _mount(cfg);
}

int SPIFlashFileSystem::check()
{
	fs.check_cb_f = [](spiffs* fs, spiffs_check_type type, spiffs_check_report report, u32_t arg1, u32_t arg2) {
		if(report > SPIFFS_CHECK_PROGRESS) {
			debug_i("SPIFFS check %d, %d, %u, %u", type, report, arg1, arg2);
		}
	};

	return SPIFFS_check(handle());
}

int SPIFlashFileSystem::getinfo(FileSystemInfo& info)
{
	info.clear();
	info.media = media;
	info.type = FileSystemType::SPIFFS;
	info.media = media;
	if(SPIFFS_mounted(handle())) {
		info.volumeID = fs.config_magic;
		bitSet(info.attr, FileSystemAttr::Mounted);
		u32_t total, used;
		SPIFFS_info(handle(), &total, &used);
		info.volumeSize = total;
		// As per SPIFFS spec
		if(used >= total) {
			bitSet(info.attr, FileSystemAttr::Check);
		} else {
			info.freeSpace = total - used;
		}
	}

	return FS_OK;
}

int SPIFlashFileSystem::geterrortext(int err, char* buffer, size_t size)
{
	int res = spiffsErrorToStr(err, buffer, size);
	if(res == 0) {
		res = IFileSystem::geterrortext(err, buffer, size);
	}

	return res;
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

file_t SPIFlashFileSystem::fopen(const FileStat& stat, FileOpenFlags flags)
{
	/*
	 * If file is marked read-only, fail write requests.
	 * Note that we trust the FileStat information provided. This provides
	 * a mechanism for an application to circumvent this flag if it becomes
	 * necessary to change  a file.
	 */
	if(bitRead(stat.attr, FileAttr::ReadOnly)) {
		return FSERR_ReadOnly;
	}

	spiffs_flags sflags;
	if(mapFileOpenFlags(flags, sflags) != 0) {
		return (file_t)FSERR_NotSupported;
	}

	auto file = SPIFFS_open_by_id(handle(), stat.id, sflags, 0);
	if(file < 0) {
		debug_ifserr(file, "fopen('%s')", stat.name);
	} else {
		cacheMeta(file);
		// File affected without write so update timestamp
		if(flags & eFO_Truncate) {
			touch(file);
		}
	}

	return file;
}

file_t SPIFlashFileSystem::open(const char* path, FileOpenFlags flags)
{
	FS_CHECK_PATH(path);
	if(path == nullptr) {
		return FSERR_BadParam;
	}

	//  debug_i("%s('%s', %s)", __FUNCTION__, fileName.c_str(), fileOpenFlagsToStr(flags).c_str());

	spiffs_flags sflags;
	if(mapFileOpenFlags(flags, sflags) != 0) {
		return (file_t)FSERR_NotSupported;
	}

	auto file = SPIFFS_open(handle(), path, sflags, 0);
	if(file < 0) {
		debug_ifserr(file, "open('%s')", path);
	} else {
		auto smb = cacheMeta(file);
		// If file is marked read-only, fail write requests !!! @todo bit late if we've got a truncate flag...
		if(smb && bitRead(smb->meta.attr, FileSystemAttr::ReadOnly) && (flags & ~eFO_ReadOnly)) {
			SPIFFS_close(handle(), file);
			return FSERR_ReadOnly;
		}

		// File affected without write so update timestamp
		if(flags & eFO_Truncate) {
			touch(file);
		}
	}

	return file;
}

int SPIFlashFileSystem::close(file_t file)
{
	if(file < 0) {
		return FSERR_FileNotOpen;
	}

	//    debug_i("%s(%d, '%s')", __FUNCTION__, m_fd, name().c_str());
	flushMeta(file);
	return SPIFFS_close(handle(), file);
}

int SPIFlashFileSystem::eof(file_t file)
{
	return SPIFFS_eof(handle(), file);
}

int32_t SPIFlashFileSystem::tell(file_t file)
{
	return SPIFFS_tell(handle(), file);
}

int SPIFlashFileSystem::truncate(file_t file, size_t new_size)
{
	return SPIFFS_ftruncate(handle(), file, new_size);
}

int SPIFlashFileSystem::flush(file_t file)
{
	flushMeta(file);
	return SPIFFS_fflush(handle(), file);
}

int SPIFlashFileSystem::read(file_t file, void* data, size_t size)
{
	int res = SPIFFS_read(handle(), file, data, size);
	if(res < 0) {
		debug_ifserr(res, "read()");
	}

	//  debug_i("read(%d): %d", bufSize, res);
	return res;
}

int SPIFlashFileSystem::write(file_t file, const void* data, size_t size)
{
	int err = SPIFFS_write(handle(), file, (void*)data, size);
	touch(file);
	return err;
}

int SPIFlashFileSystem::lseek(file_t file, int offset, SeekOriginFlags origin)
{
	int res = SPIFFS_lseek(handle(), file, offset, origin);
	if(res < 0) {
		debug_ifserr(res, "lseek()");
		return res;
	}

	//  debug_i("Seek(%d, %d): %d", offset, origin, res);

	return res;
}

SpiffsMetaBuffer* SPIFlashFileSystem::cacheMeta(file_t file)
{
	SpiffsMetaBuffer* smb;
	if(getMeta(file, smb) < 0) {
		return nullptr;
	}

	memset(smb->buffer, 0xFF, sizeof(SpiffsMetaBuffer));

	spiffs_stat stat;
	int res = SPIFFS_fstat(handle(), file, &stat);
	//    debug_hex(DBG, "Meta", stat.meta, SPIFFS_OBJ_META_LEN);

	if(res >= 0) {
		memcpy(smb, stat.meta, sizeof(stat.meta));
	}

	checkMeta(smb->meta);

	return smb;
}

int SPIFlashFileSystem::getMeta(file_t file, SpiffsMetaBuffer*& meta)
{
	unsigned off = SPIFFS_FH_UNOFFS(handle(), file) - 1;
	if(off >= FFS_MAX_FILEDESC) {
		debug_e("getMeta(%d) - bad file", file);
		return FSERR_InvalidHandle;
	}
	meta = &metaCache[off];
	return FS_OK;
}

/*
 * If metadata has changed, flush it to SPIFFS.
 */
int SPIFlashFileSystem::flushMeta(file_t file)
{
	SpiffsMetaBuffer* smb;
	int res = getMeta(file, smb);
	if(res < 0) {
		return res;
	}

	// Changed ?
	if(smb->meta.isDirty()) {
		debug_i("Flushing Metadata to disk");
		smb->meta.clearDirty();
		int res = SPIFFS_fupdate_meta(handle(), file, smb);
		if(res < 0) {
			debug_ifserr(res, "fupdate_meta()");
		}
	}

	return res;
}

int SPIFlashFileSystem::stat(const char* path, FileStat* stat)
{
	spiffs_stat ss;
	int res = SPIFFS_stat(handle(), path, &ss);
	if(res < 0) {
		return res;
	}

	if(stat) {
		stat->clear();
		stat->fs = this;
		stat->name.copy(reinterpret_cast<const char*>(ss.name));
		stat->size = ss.size;
		stat->id = ss.obj_id;
		FileMeta meta;
		memcpy(&meta, ss.meta, sizeof(meta));
		checkMeta(meta);
		copyMeta(*stat, meta);
	}

	return res;
}

int SPIFlashFileSystem::fstat(file_t file, FileStat* stat)
{
	spiffs_stat ss;
	int res = SPIFFS_fstat(handle(), file, &ss);
	if(res < 0) {
		return res;
	}

	SpiffsMetaBuffer* smb;
	res = getMeta(file, smb);
	if(res < 0) {
		return res;
	}

	memcpy(smb, ss.meta, sizeof(SpiffsMetaBuffer));
	checkMeta(smb->meta);

	if(stat) {
		stat->clear();
		stat->fs = this;
		stat->name.copy(reinterpret_cast<const char*>(ss.name));
		stat->size = ss.size;
		stat->id = ss.obj_id;
		copyMeta(*stat, smb->meta);
	}

	return res;
}

int SPIFlashFileSystem::setacl(file_t file, FileACL* acl)
{
	if(acl == nullptr) {
		return FSERR_BadParam;
	}

	SpiffsMetaBuffer* smb;
	int res = getMeta(file, smb);
	if(res >= 0) {
		smb->meta.acl = *acl;
		smb->meta.setDirty();
	}
	return res;
}

int SPIFlashFileSystem::setattr(file_t file, FileAttributes attr)
{
	SpiffsMetaBuffer* smb;
	int res = getMeta(file, smb);
	if(res >= 0 && smb->meta.attr != attr) {
		smb->meta.setDirty();
	}
	return res;
}

int SPIFlashFileSystem::settime(file_t file, time_t mtime)
{
	SpiffsMetaBuffer* smb = nullptr;
	int res = getMeta(file, smb);
	if(res >= 0 && smb->meta.mtime != mtime) {
		smb->meta.mtime = mtime;
		smb->meta.setDirty();
	}
	return res;
}

int SPIFlashFileSystem::opendir(const char* path, filedir_t* dir)
{
	if(dir == nullptr) {
		return FSERR_BadParam;
	}

	FS_CHECK_PATH(path);
	unsigned pathlen = 0;
	if(path != nullptr) {
		pathlen = strlen(path);
		if(pathlen >= sizeof(FileDir::path)) {
			return FSERR_NameTooLong;
		}
	}

	auto d = new FileDir;
	if(d == nullptr) {
		return FSERR_NoMem;
	}

	if(SPIFFS_opendir(handle(), nullptr, &d->d) == nullptr) {
		debug_ifserr(SPIFFS_errno(handle()), "opendir");
		delete d;
		return SPIFFS_errno(handle());
	}

	if(pathlen != 0) {
		memcpy(d->path, path, pathlen);
	}
	d->path[pathlen] = '\0';
	d->pathlen = pathlen;

	*dir = d;
	return FS_OK;
}

int SPIFlashFileSystem::readdir(filedir_t dir, FileStat* stat)
{
	if(dir == nullptr) {
		return FSERR_BadParam;
	}

	spiffs_dirent e;
	for(;;) {
		if(SPIFFS_readdir(&dir->d, &e) == nullptr) {
			return SPIFFS_errno(handle());
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

		if(stat) {
			stat->clear();
			stat->fs = this;
			stat->name.copy(name);
			if(nextSep != nullptr) {
				bitSet(stat->attr, FileAttr::Directory);
			} else {
				stat->size = e.size;
				stat->id = e.obj_id;
				FileMeta meta;
				memcpy(&meta, e.meta, sizeof(meta));
				checkMeta(meta);
				copyMeta(*stat, meta);
			}
		}

		return FS_OK;
	}
}

int SPIFlashFileSystem::closedir(filedir_t dir)
{
	if(dir == nullptr) {
		return FSERR_BadParam;
	}

	int res = SPIFFS_closedir(&dir->d);
	delete dir;
	return res;
}

int SPIFlashFileSystem::rename(const char* oldpath, const char* newpath)
{
	FS_CHECK_PATH(oldpath);
	FS_CHECK_PATH(newpath);
	if(oldpath == nullptr || newpath == nullptr) {
		return FSERR_BadParam;
	}

	return SPIFFS_rename(handle(), oldpath, newpath);
}

int SPIFlashFileSystem::remove(const char* path)
{
	FS_CHECK_PATH(path);
	if(path == nullptr) {
		return FSERR_BadParam;
	}

	auto res = SPIFFS_remove(handle(), path);
	debug_ifserr(res, "remove('%s')", path);
	return res;
}

int SPIFlashFileSystem::fremove(file_t file)
{
	return SPIFFS_fremove(handle(), file);
}

int SPIFlashFileSystem::isfile(file_t file)
{
	SpiffsMetaBuffer* meta;
	return getMeta(file, meta);
}

int SPIFlashFileSystem::getFilePath(fileid_t fileid, NameBuffer& buffer)
{
	auto fs = handle();
	spiffs_block_ix block_ix;
	int lu_entry;
	int res = spiffs_obj_lu_find_id(fs, 0, 0, fileid | SPIFFS_OBJ_ID_IX_FLAG, &block_ix, &lu_entry);
	if(res == SPIFFS_OK) {
		spiffs_page_object_ix_header objix_hdr;
		spiffs_page_ix pix = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(fs, block_ix, lu_entry);
		res = _spiffs_rd(fs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, SPIFFS_PAGE_TO_PADDR(fs, pix),
						 sizeof(objix_hdr), reinterpret_cast<u8_t*>(&objix_hdr));
		if(res == SPIFFS_OK) {
			res = buffer.copy(reinterpret_cast<const char*>(objix_hdr.name));
		}
	}

	return res;
}
