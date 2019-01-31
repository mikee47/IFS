/*
 * SPIFFSObjectStore.cpp
 *
 *  Created on: 2 Sep 2018
 *      Author: mikee47
 */

#define DEBUG_BUILD 1
#undef DEBUG_VERBOSE_LEVEL
#define DEBUG_VERBOSE_LEVEL 3

#include "SPIFFSObjectStore.h"
#include "spiffs_sming.h"

/*
 * Number of pages to cache
 */
#define CACHE_PAGES 8

#define WORK_BUF_SIZE(_log_page_size) ((_log_page_size)*2)

#define FDS_BUF_SIZE(_num_filedesc) ((_num_filedesc) * sizeof(spiffs_fd))

#define CACHE_SIZE(_pages) (sizeof(spiffs_cache) + (_pages) * (sizeof(spiffs_cache_page) + cfg.log_page_size))

#define SPIFFS_LOG_PAGE_SIZE 256

// Use these values for special object IDs as SPIFFS is unlikely to have used them - must be less than 0x8000
static const FWObjectID spiffsos_Volume = 0x7fff;
static const FWObjectID spiffsos_Root = 0x7ffe;

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

static void getRootObject(FWFS_Object& obj)
{
	obj._type = fwobt_Directory;
	obj.data16._contentSize = sizeof(obj.data16.named); // + sizeof child objects, _objectCount, namelen, etc.
}

static void fillHeader(FWObjDesc& od, const spiffs_page_object_ix_header& objix_hdr)
{
	od.obj._type = fwobt_File;
	od.ref.id = objix_hdr.p_hdr.obj_id & ~SPIFFS_OBJ_ID_IX_FLAG;
	auto& obj = od.obj.data16;
	obj.named.namelen = strlen((const char*)objix_hdr.name);
	obj.named.mtime = 0;
	obj._contentSize = sizeof(obj.named) + obj.named.namelen;
	// @todo also needs a data object
}

SPIFFSObjectStore::~SPIFFSObjectStore()
{
	SPIFFS_unmount(&_fs);
	delete[] _cache;
	delete[] _fds;
	delete[] _work_buf;
	delete _media;
}

int SPIFFSObjectStore::initialise()
{
	int res = mount();
	if(res < 0)
		return res;

	//	enumObjects();

	return res;
}

int SPIFFSObjectStore::mounted(const FWObjDesc& od)
{
	return FS_OK;
}

spiffs_file SPIFFSObjectStore::handleAddRef(spiffs_file handle)
{
	auto fh = SPIFFS_FH_UNOFFS(fs(), handle);

	if(fh > 0 && fh <= FFS_MAX_FILEDESC) {
		auto& refCount = _handleRefs[fh - 1];
		++refCount;
		debug_i("addref(%u) = %u", handle, refCount);
	}

	return handle;
}

int SPIFFSObjectStore::handleRelease(spiffs_file handle)
{
	int res = FS_OK;
	auto fh = SPIFFS_FH_UNOFFS(fs(), handle);

	if(fh > 0 && fh <= FFS_MAX_FILEDESC) {
		auto& refCount = _handleRefs[fh - 1];
		--refCount;
		if(refCount == 0) {
			res = SPIFFS_close(fs(), handle);
			debug_i("SPIFFS_close(%u) returned %d", handle, res);
		}
		debug_i("release(%u) = %u", handle, refCount);
	}

	return res;
}

int SPIFFSObjectStore::open(FWObjDesc& od)
{
	spiffs_fd* fd;
	int res = spiffs_fd_find_new(fs(), &fd, 0);
	SPIFFS_CHECK_RES(res);

	_handleRefs[fd->file_nbr - 1] = 0;

	spiffs_flags flags = SPIFFS_O_RDONLY;
	res = spiffs_object_open_by_id(fs(), od.ref.id, fd, flags, 0);
	if(res >= 0) {
		spiffs_page_object_ix_header objix_hdr;
		// Get object header info
		res = _spiffs_rd(fs(), SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_READ, fd->file_nbr,
						 SPIFFS_PAGE_TO_PADDR(fs(), fd->objix_hdr_pix), sizeof(objix_hdr), (u8_t*)&objix_hdr);

		if(res >= 0) {
			fillHeader(od, objix_hdr);
			od.ref.handle = SPIFFS_FH_OFFS(fs(), fd->file_nbr);
			od.ref.offset = 0;
			handleAddRef(od.ref.handle);
		}
	}

	if(res < 0)
		spiffs_fd_return(fs(), fd->file_nbr);

	return res;

	/*
	 * Here's a thing. We keep offsets in the descriptor but for SPIFFS we have a file handle,
	 * hence descriptor, hence file position and other stuff. If we add a seek() method to the
	 * object store we can keep our file handle, or a pointer to the descriptor, in the offset
	 * field. That's something we can look at later though.
	 */
}

int SPIFFSObjectStore::openChild(const FWObjDesc& parent, const FWObjDesc& child, FWObjDesc& od)
{
	debug_i("SPIFFSObjectStore::open(%d, %u)", child.ref.handle, child.ref.id);

	if(!child.obj.isRef()) {
		od = child;
		od.ref.handle = handleAddRef(parent.ref.handle);
		od.ref.offset += parent.childTableOffset();
		return FS_OK;
	}

	od.ref = FWObjRef(child.ref, child.obj.data8.ref.id);
	int res = open(od);

	return res;
}

int SPIFFSObjectStore::close(FWObjDesc& od)
{
	debug_i("SPIFFSObjectStore::close(%d, %u)", od.ref.handle, od.ref.id);

	handleRelease(od.ref.handle);

	return FS_OK;
}

// We just need to filter out current objects here
static s32_t readObject_v(spiffs* fs, spiffs_obj_id obj_id, spiffs_block_ix bix, int ix_entry, const void* user_const_p,
						  void* user_var_p)
{
	if(obj_id == SPIFFS_OBJ_ID_FREE || obj_id == SPIFFS_OBJ_ID_DELETED || (obj_id & SPIFFS_OBJ_ID_IX_FLAG) == 0)
		return SPIFFS_VIS_COUNTINUE;

	return SPIFFS_OK;
}

// od.offset must be positioned on a valid index page
int SPIFFSObjectStore::readIndexHeader(const FWObjDesc& od, spiffs_page_object_ix_header& objix_hdr)
{
	debug_i("readIndexHeader(%d, 0x%08X, %u)", od.ref.handle, od.ref.offset, od.ref.id);

	assert(od.ref.handle == 0); // Core objects correspond to SPIFFS objects

	spiffs_block_ix bix = od.ref.offset / SPIFFS_CFG_LOG_BLOCK_SZ(fs());
	int ix_entry = (od.ref.offset % SPIFFS_CFG_LOG_BLOCK_SZ(fs())) / SPIFFS_CFG_LOG_PAGE_SZ(fs());

	spiffs_page_ix pix = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(fs(), bix, ix_entry);
	uint32_t paddr = SPIFFS_PAGE_TO_PADDR(fs(), pix);
	int res = _spiffs_rd(fs(), SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, paddr, sizeof(objix_hdr),
						 reinterpret_cast<u8_t*>(&objix_hdr));

	// We get this value if file hasn't been written yet
	if(objix_hdr.size == SPIFFS_UNDEFINED_LEN)
		objix_hdr.size = 0;

	/*
	if (res >= 0) {
		debug_i("SPIFFS 0x%08X, id = %u, bix = %u, entry = %u, type = %u, flags = 0x%02x, size = %d, name = %s",
				 paddr, objix_hdr.p_hdr.obj_id & ~SPIFFS_OBJ_ID_IX_FLAG, bix, ix_entry, objix_hdr.type, objix_hdr.p_hdr.flags, objix_hdr.size, objix_hdr.name);
	}
*/

	return res;
}

int SPIFFSObjectStore::findIndexHeader(FWObjDesc& od, spiffs_page_object_ix_header& objix_hdr)
{
	debug_i("findIndexHeader(%d, 0x%08X, %u)", od.ref.handle, od.ref.offset, od.ref.id);
	++od.ref.readCount;

	// We don't account for lookup pages, but that doesn't matter; we just use it to translate bix/pix <-> ref.offset

	auto lpsz = SPIFFS_CFG_LOG_PAGE_SZ(fs());
	// If offset isn't on a logical page start (because od.next() was called), skip to the next entry start position
	if(od.ref.offset % lpsz) {
		//		debug_i("Adjusting offset from 0x%08X to 0x%08X", od.ref.offset, od.ref.offset + lpsz);
		od.ref.offset += lpsz;
	}

	spiffs_block_ix bix = od.ref.offset / SPIFFS_CFG_LOG_BLOCK_SZ(fs());
	int ix_entry = (od.ref.offset % SPIFFS_CFG_LOG_BLOCK_SZ(fs())) / SPIFFS_CFG_LOG_PAGE_SZ(fs());
	int res = spiffs_obj_lu_find_entry_visitor(fs(), bix, ix_entry, SPIFFS_VIS_NO_WRAP, 0, readObject_v, 0, nullptr,
											   &bix, &ix_entry);

	if(res == SPIFFS_VIS_END)
		res = FSERR_EndOfObjects;
	else if(res >= 0) {
		// Update the descriptor offset to the actual entry position
		od.ref.offset = (bix * SPIFFS_CFG_LOG_BLOCK_SZ(fs())) + (ix_entry * SPIFFS_CFG_LOG_PAGE_SZ(fs()));

		// And read the header
		res = readIndexHeader(od, objix_hdr);
	}

	return res;
}

int SPIFFSObjectStore::readHeader(FWObjDesc& od)
{
	//	debug_i("readHeader(%d, 0x%08X, %u)", od.ref.handle, od.ref.offset, od.ref.id);

	assert(od.ref.handle == 0); // Core objects correspond to SPIFFS objects

	/*
	 * Right. We get called here immediately after mount() with an empty descriptor.
	 * i.e. no files are actually open.
	 *
	 * FWFS wants to scan through all objects on the system. So we need to open an
	 * object enumeration. Be nice to do this at a lower level than opendir/readdir,
	 * especially as we're not using names if at all possible.
	 *
	 * Every object on the SPIFFS volume is ours. On a freshly formatted volume there
	 * won't be anything, so we could just return FSERR_EndOfObjects.
	 *
	 * However, we expect to see a volume object, containing one directory object which
	 * is where we start.
	 *
	 *
	 * NOTE: We can always offset FWFS_ObjectType so it doesn't conflict with hydrogen,
	 * although it shouldn't matter as volume will be formatted specifically for FWFS.
	 */

	FWFS_Object objRoot;
	getRootObject(objRoot);

	FWFS_Object objVolume = {};
	objVolume._type = fwobt_Volume;
	objVolume.data16._contentSize = objRoot.size();

	if(od.ref.offset == 0) {
		od.obj = objVolume;
		od.ref.id = spiffsos_Volume;
		++od.ref.readCount;
		return FS_OK;
	}

	if(od.ref.offset == objVolume.childTableOffset()) {
		od.obj = objRoot;
		od.ref.id = spiffsos_Root;
		++od.ref.readCount;
		return FS_OK;
	}

	spiffs_page_object_ix_header objix_hdr;
	int res = findIndexHeader(od, objix_hdr);
	if(res == FSERR_EndOfObjects) {
		od.obj = {};
		od.obj._type = fwobt_End;
		od.ref.id = 0x7ff2;
		return FS_OK;
	}

	if(res >= 0)
		fillHeader(od, objix_hdr);

	return res;
}

int SPIFFSObjectStore::readChildHeader(const FWObjDesc& parent, FWObjDesc& child)
{
	debug_i("readChildHeader(%d, %u, 0x%08X, %u)", parent.ref.handle, parent.ref.id, child.ref.offset, child.ref.id);

	int res;

	if(parent.ref.handle == 0) {
		// parent = volume object
		if(parent.ref.id == spiffsos_Volume) {
			getRootObject(child.obj);
			child.ref.id = spiffsos_Root;
			return FS_OK;
		}

		if(parent.ref.id == spiffsos_Root) {
			spiffs_page_object_ix_header objix_hdr;
			res = findIndexHeader(child, objix_hdr);
			if(res >= 0)
				fillHeader(child, objix_hdr);
			return res;
		}

		/*
		 * Other objects have a SPIFFS-allocated object ID
		 *
		 */
		if(child.ref.offset != 0) {
			debug_i("Only one child data object for now");
			return FSERR_EndOfObjects;
		}

		spiffs_page_object_ix_header objix_hdr;
		res = readIndexHeader(parent, objix_hdr);
		if(res >= 0) {
			child.obj._type = fwobt_Data24;
			child.obj.data24.setContentSize(objix_hdr.size);
			child.ref.id = 0;
		}

		return FS_OK;
	}

	/*
	 * Handle open.
	 *
	 * For now, the content starts with the object table directly; other fields are taken from SPIFFS index page.
	 * So we need to fake a child data object. For simplicity, let's make this a data24 object.
	 */

	spiffs_fd* fd;
	res = spiffs_fd_get(fs(), SPIFFS_FH_UNOFFS(fs(), parent.ref.handle), &fd);
	SPIFFS_CHECK_RES(res);

	if(child.ref.offset == 0) {
		child.obj._type = fwobt_Data24;
		child.obj.data24.setContentSize(fd->size);
		child.ref.id = 0;
	} else
		res = FSERR_EndOfObjects;

	return res;
}

int SPIFFSObjectStore::readContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer)
{
	debug_i("readContent(%d, 0x%08X, %u)", od.ref.handle, od.ref.offset, od.ref.id);

	// Is this a SPIFFS object ?
	if(od.ref.handle == 0) {
		assert(od.obj.isNamed());

		spiffs_page_object_ix_header objix_hdr;
		int res = readIndexHeader(od, objix_hdr);
		if(res >= 0) {
			if(offset == od.obj.data16.named.nameOffset()) {
				assert(size <= SPIFFS_OBJ_NAME_LEN);
				memcpy(buffer, objix_hdr.name, size);
				return FS_OK;
			}

			// Whilst we _could_get called with offset < nameOffset(), we don't - those fields get returned by readHeader()

			/* OK, so when reading child content we need the parent object handle. Options:
			 *
			 * 	- Pass both parent & child objects to this method
			 * 	- Duplicate parent handle in child (2 handles open)
			 * 	- Include reference to parent in descriptor
			 *
			 * 	We only need the parent if we haven't got a file handle, so perhaps it's a dual-use field?
			 */

			// Child objects...
			return FSERR_NotImplemented;
		}
	}

	/*
			// Data object content
			auto offset = child.ref.offset - child.obj.data24.contentOffset();
			res = SPIFFS_lseek(fs(), parent.ref.handle, offset, SEEK_SET);
			if (res >= 0)
				res = SPIFFS_read(fs(), parent.ref.handle, &child.obj, sizeof(child.obj));
	*/

	int res = SPIFFS_lseek(fs(), od.ref.handle, offset, SPIFFS_SEEK_SET);
	if(res >= 0)
		res = SPIFFS_read(fs(), od.ref.handle, buffer, size);

	return res;
}

int SPIFFSObjectStore::writeHeader(FWObjDesc& od)
{
	/*
	 * OK, how to write objects. These are the operations we need to support:
	 *
	 * 	Create named object - SPIFFS_creat
	 * 	Append child object - append to file
	 * 	Remove entry from object: mark as deleted, rewrite object on close/commit
	 *	Rename object - rewrite contents, using same object ID.
	 *  Delete object - remove all child objects, then call SPIFFS_remove. This would
	 *  	be handled via FWFS, we'd just do the object remove.
	 *
	 *	We should reserve 0 to indicate a deleted object; we can bypass SPIFFS and
	 *	mark it directly using a single flash write. Still housekeeping to consider though.
	 *
	 */

	return FSERR_NotImplemented;
}

int SPIFFSObjectStore::writeContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer)
{
	return FSERR_NotImplemented;
}

int SPIFFSObjectStore::mount()
{
	if(!_media)
		return FSERR_NoMedia;

	uint32_t blockSize = _media->blockSize();
	if(blockSize < MIN_BLOCKSIZE)
		blockSize = Align(MIN_BLOCKSIZE, blockSize);

	_fs.user_data = _media;
	spiffs_config cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.hal_read_f = f_read;
	cfg.hal_write_f = f_write;
	cfg.hal_erase_f = f_erase;
	cfg.phys_size = _media->mediaSize();
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

	_work_buf = new uint8_t[WORK_BUF_SIZE(cfg.log_page_size)];
	_fds = new uint8_t[FDS_BUF_SIZE(FFS_MAX_FILEDESC)];
	_cache = new uint8_t[CACHE_SIZE(CACHE_PAGES)];

	if(!_work_buf || !_fds || !_cache) {
		return FSERR_NoMem;
	}

	auto res = _mount(cfg);
	if(res < 0) {
		/*
     * Mount failed, so we either try to repair the system or format it.
     * For now, just format it.
     */

		return res;

		//		res = format();
	}

#if SPIFFS_TEST_VISUALISATION
	if(res == SPIFFS_OK)
		SPIFFS_vis(&_fs);
#endif

	return res;
}

int SPIFFSObjectStore::_mount(spiffs_config& cfg)
{
	auto res = SPIFFS_mount(fs(), &cfg, _work_buf, _fds, FDS_BUF_SIZE(FFS_MAX_FILEDESC), _cache,
							CACHE_SIZE(CACHE_PAGES), nullptr);
	return res;
}

/*
 * Format the file system and leave it mounted in an accessible state.
 */
int SPIFFSObjectStore::format()
{
	spiffs_config cfg = _fs.cfg;
	// Must be unmounted before format is called - see API
	SPIFFS_unmount(fs());
	int res = SPIFFS_format(fs());
	if(res < 0)
		return res;

	// Re-mount
	return _mount(cfg);
}

/*
 * When FWFS does .next() on object descriptor, it moves the offset to the first byte after the object.
 * For child objects contained within SPIFFS files that's fine, but what about 'root' objects? If we combined
 * bix/entry into a 32-bit offset value, would that be sufficient?
 *
 * Config (above) specifies:
 *
 * 	Logical block size: 8192 bytes (2 x flash blocks)
 * 	Logical sector size: 256 bytes
 * 	Pages per block = 8192 / 256 = 32
 *
 * 	offset = (bix * cfg.log_block_size) + (ix_entry * cfg.log_page_size)
 *
 *
 *
 */
static s32_t enumObject_v(spiffs* fs, spiffs_obj_id obj_id, spiffs_block_ix bix, int ix_entry, const void* user_const_p,
						  void* user_var_p)
{
	spiffs_page_object_ix_header objix_hdr;
	if(obj_id == SPIFFS_OBJ_ID_FREE || obj_id == SPIFFS_OBJ_ID_DELETED || (obj_id & SPIFFS_OBJ_ID_IX_FLAG) == 0)
		return SPIFFS_VIS_COUNTINUE;

	auto objectCount = reinterpret_cast<spiffs_page_ix*>(user_var_p);
	++(*objectCount);

	spiffs_page_ix pix = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(fs, bix, ix_entry);
	int res = _spiffs_rd(fs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, SPIFFS_PAGE_TO_PADDR(fs, pix),
						 sizeof(objix_hdr), reinterpret_cast<u8_t*>(&objix_hdr));
	if(res < 0)
		return res;

	debug_i("SPIFFS 0x%08X, id = %u, bix = %u, entry = %u, type = %u, flags = 0x%02x, size = %d, name = %s",
			SPIFFS_OBJ_LOOKUP_ENTRY_TO_PADDR(fs, bix, ix_entry), obj_id & ~SPIFFS_OBJ_ID_IX_FLAG, bix, ix_entry,
			objix_hdr.type, objix_hdr.p_hdr.flags, objix_hdr.size, objix_hdr.name);

	return SPIFFS_VIS_COUNTINUE;
}

int SPIFFSObjectStore::enumObjects()
{
	SPIFFS_API_DBG("%s\n", __func__);
	if(!SPIFFS_CHECK_MOUNT(fs()))
		return SPIFFS_ERR_NOT_MOUNTED;

	_objectCount = 0;
	spiffs_block_ix bix = 0;
	int entry = 0;
	int res = spiffs_obj_lu_find_entry_visitor(fs(), bix, entry, SPIFFS_VIS_NO_WRAP, 0, enumObject_v, nullptr,
											   &_objectCount, &bix, &entry);
	return res;
}
