/**
 * ObjectStore.h
 *
 * Created on: 1 Sep 2018
 *
 * Copyright 2019 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the IFS Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ****/

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
		return flags[Flag::mounted];
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
	enum class Flag {
		mounted,
	};

	Storage::Partition partition;
	ObjRef lastFound;
#ifdef FWFS_OBJECT_CACHE
	ObjRefCache cache;
#endif
	BitSet<uint8_t, Flag> flags;
};

} // namespace FWFS
} // namespace IFS
