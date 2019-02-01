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

	~FWObjectStore() override
	{
		delete _media;
	}

	int initialise() override;
	int mounted(const FWObjDesc& od) override;

	bool isMounted() override
	{
		return _mounted;
	}

	int open(FWObjDesc& od) override;
	int openChild(const FWObjDesc& parent, const FWObjDesc& child, FWObjDesc& od) override;
	int readHeader(FWObjDesc& od) override;
	int readChildHeader(const FWObjDesc& parent, FWObjDesc& child) override;
	int readContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer) override;
	int close(FWObjDesc& od) override;

	IFSMedia* getMedia() override
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
