/*
 * SPIFFSObjectStore.h
 *
 *  Created on: 2 Sep 2018
 *      Author: mikee47
 *
 */

#ifndef __SPIFFS_OBJECT_STORE_H
#define __SPIFFS_OBJECT_STORE_H

#include "IFSObjectStore.h"
#include "IFSMedia.h"
#include "spiffs.h"

extern "C" {
#include "spiffs_nucleus.h"
}

/*
 * Maxmimum number of open files
 */
#define FFS_MAX_FILEDESC 8

/** @brief object store for SPIFFS-based filesystem
 *
 *	@note
 *
 *	SPIFFS stores filenames in an object's index page, which also has an optional
 *	metadata space; this could be used for mtime, but it doesn't have to be.
 *	Let's assume we're staying with 256-byte pages. It's probably best to limit
 *	filename length, say 32 characters. That leaves about 210 bytes in the index
 *	page to store the FWFS object table; access by setting SPIFFS_OBJ_META_LEN appropriately.
 *	That's room for around 50 child objects, assuming there's no immediate data.
 *	Data content would be placed in SPIFFS data pages anyway. For directories, which might
 *	need more space for the object table, using file content pages would provide more space.
 */
class SPIFFSObjectStore : public IFSObjectStore
{
public:
	SPIFFSObjectStore(IFSMedia* media) : _media(media)
	{
	}

	virtual ~SPIFFSObjectStore();

	virtual int initialise();
	virtual int mounted(const FWObjDesc& od);
	virtual bool isMounted()
	{
		return SPIFFS_mounted(fs());
	}
	virtual int open(FWObjDesc& od);
	virtual int openChild(const FWObjDesc& parent, const FWObjDesc& child, FWObjDesc& od);
	virtual int readHeader(FWObjDesc& od);
	virtual int readChildHeader(const FWObjDesc& parent, FWObjDesc& child);
	virtual int readContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer);
	virtual int writeHeader(FWObjDesc& od);
	virtual int writeContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer);
	virtual int close(FWObjDesc& od);

	virtual IFSMedia* getMedia()
	{
		return _media;
	}

private:
	spiffs* fs()
	{
		return &_fs;
	}

	int _mount(spiffs_config& cfg);
	int mount();
	int format();

	int readIndexHeader(const FWObjDesc& od, spiffs_page_object_ix_header& objix_hdr);
	int findIndexHeader(FWObjDesc& od, spiffs_page_object_ix_header& objix_hdr);
	int enumObjects();

	spiffs_file handleAddRef(spiffs_file handle);
	int handleRelease(spiffs_file handle);

private:
	IFSMedia* _media = nullptr;
	spiffs _fs = {};
	uint8_t* _work_buf = nullptr;
	uint8_t* _fds = nullptr;
	u8_t* _cache = nullptr;
	spiffs_page_ix _objectCount = 0;	   ///< Number of objects (not currently used)
	uint8_t _handleRefs[FFS_MAX_FILEDESC]; ///< For sharing SPIFFS handles
};

#endif // __SPIFFS_OBJECT_STORE_H
