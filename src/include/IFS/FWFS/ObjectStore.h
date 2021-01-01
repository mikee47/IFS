/*
 * ObjectStore.h
 *
 *  Created on: 1 Sep 2018
 *      Author: mikee47
 *
 */

#pragma once

#include "../ObjectStore.h"

#ifdef FWFS_OBJECT_CACHE
#include "ObjRefCache.h"
#endif

namespace IFS
{
namespace FWFS
{
/**
 * @brief object store for read-only filesystem
 */
class ObjectStore : public IObjectStore
{
public:
	ObjectStore(Storage::Partition partition) : partition(partition)
	{
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

	Storage::Partition& getPartition() override
	{
		return partition;
	}

private:
	Storage::Partition partition;
	ObjRef lastFound;
	struct Flags {
		bool mounted;
	};
	Flags flags{};
#ifdef FWFS_OBJECT_CACHE
	ObjRefCache cache;
#endif
};

} // namespace FWFS
} // namespace IFS
