/**
 * ObjRefCache.h
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

#include "ObjectStore.h"

namespace IFS
{
namespace FWFS
{
#define FWFS_OBJECT_CACHE

// Add a cache entry at regular index positions - pick a number here which is a modulus of 2
#define FWFS_CACHE_SPACING 8
#define FWFS_CACHE_MASK (FWFS_CACHE_SPACING - 1)

#ifdef FWFS_OBJECT_CACHE

/** @brief Cache the locations of several objects to improve search speed
 */
class ObjRefCache
{
public:
	void initialise(Object::ID objectCount)
	{
		clear();
		if(objectCount < FWFS_CACHE_SPACING) {
			return;
		}
		offsetCount = (objectCount / FWFS_CACHE_SPACING) - 1;
		offsets.reset(new uint32_t[offsetCount]);
		if(!offsets) {
			offsetCount = 0;
		}
	}

	void clear()
	{
		offsetCount = 0;
		offsets.reset();
	}

	void add(const ObjRef& ref)
	{
		if(ref.id && (ref.id & FWFS_CACHE_MASK) == 0) {
			auto po = getOffset(ref.id);
			if(po != 0) {
				debug_d("Caching #%u @ 0x%08X", ref.id, ref.offset);
				*po = ref.offset;
			}
		}
	}

	uint32_t* getOffset(FWFS::Object::ID objIndex)
	{
		int pos = (objIndex / FWFS_CACHE_SPACING) - 1;
		return (pos >= 0 && pos < offsetCount) ? &offsets[pos] : nullptr;
	}

	/** @brief see if the cache can get a better search position to find an object
	 *  @param ref current object reference, must be valid
	 *  @param objID object we're looking for
	 *  @note FWRO object IDs are sequential indices, so we'll try to get the start offset as close as possible
	 */
	void improve(ObjRef& ref, FWFS::Object::ID objID)
	{
		FWFS::Object::ID cachedIndex = objID & ~FWFS_CACHE_MASK;
		if(cachedIndex <= ref.id) {
			return;
		}

		auto po = getOffset(cachedIndex);
		if(po != 0) {
			debug_d("Cache hit #%u @ 0x%08X (for #%u)", cachedIndex, *po, objID);
			ref.id = cachedIndex;
			ref.offset = *po;
		}
	}

private:
	std::unique_ptr<uint32_t> offsets;
	FWFS::Object::ID offsetCount{0}; ///< Number of cached offsets
};

#endif

} // namespace FWFS
} // namespace IFS
