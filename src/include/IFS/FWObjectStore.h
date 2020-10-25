/*
 * ObjectStore.h
 *
 *  Created on: 1 Sep 2018
 *      Author: mikee47
 *
 */

#pragma once

#include "ObjectStore.h"
#include "Media.h"

#ifdef FWFS_OBJECT_CACHE
#include "FWObjRefCache.h"
#endif

namespace IFS
{
/** @brief object store for read-only filesystem
 *
 */
class FWObjectStore : public ObjectStore
{
public:
	FWObjectStore(Media* media) : media(media)
	{
	}

	~FWObjectStore() override
	{
		delete media;
	}

	int initialise() override;
	int mounted(const FWObjDesc& od) override;

	bool isMounted() override
	{
		return flags.mounted;
	}

	int open(FWObjDesc& od) override;
	int openChild(const FWObjDesc& parent, const FWObjDesc& child, FWObjDesc& od) override;
	int readHeader(FWObjDesc& od) override;
	int readChildHeader(const FWObjDesc& parent, FWObjDesc& child) override;
	int readContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer) override;
	int close(FWObjDesc& od) override;

	Media* getMedia() override
	{
		return media;
	}

private:
	Media* media = nullptr;
	FWObjRef lastFound;
	struct Flags {
		bool mounted;
	};
	Flags flags{};
#ifdef FWFS_OBJECT_CACHE
	FWObjRefCache cache;
#endif
};

} // namespace IFS
