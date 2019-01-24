/*
 * IFSObjectStore.h
 *
 *  Created on: 1 Sep 2018
 *      Author: mikee47
 *
 */

#ifndef __FW_OBJECT_STORE_H
#define __FW_OBJECT_STORE_H

#include "IFSObjectStore.h"
#include "IFSMedia.h"

#ifdef FWFS_OBJECT_CACHE
#include "FWObjRefCache.h"
#endif

/** @brief object store for read-only filesystem
 *
 */
class FWObjectStore : public IFSObjectStore
{
public:
	FWObjectStore(IFSMedia* media) : _media(media)
	{
	}

	virtual ~FWObjectStore()
	{
		delete _media;
	}

	virtual int initialise();
	virtual int mounted(const FWObjDesc& od);
	virtual bool isMounted()
	{
		return _mounted;
	}
	virtual int open(FWObjDesc& od);
	virtual int openChild(const FWObjDesc& parent, const FWObjDesc& child, FWObjDesc& od);
	virtual int readHeader(FWObjDesc& od);
	virtual int readChildHeader(const FWObjDesc& parent, FWObjDesc& child);
	virtual int readContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer);
	virtual int close(FWObjDesc& od);

	virtual IFSMedia* getMedia()
	{
		return _media;
	}

private:
	IFSMedia* _media = nullptr;
	FWObjRef _lastFound;
	bool _mounted = false;
#ifdef FWFS_OBJECT_CACHE
	FWObjRefCache _cache;
#endif
};

#endif // __FW_OBJECT_STORE_H
