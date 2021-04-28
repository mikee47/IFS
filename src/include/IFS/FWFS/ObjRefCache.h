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

#include "Object.h"

namespace IFS
{
namespace FWFS
{
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

	void update(const ObjRef& ref)
	{
		if(ref.id == 0) {
			return;
		}
		if(ref.id % FWFS_CACHE_SPACING != 0) {
			return;
		}
		auto po = getOffsetPtr(ref.id);
		if(po == nullptr) {
			return;
		}
#if FWFS_DEBUG
		debug_d("Caching #%u @ 0x%08X", ref.id, ref.offset);
#endif
		*po = ref.offset;
	}

	uint32_t* getOffsetPtr(FWFS::Object::ID objId)
	{
		int pos = (objId / FWFS_CACHE_SPACING) - 1;
		return (pos >= 0 && pos < offsetCount) ? &offsets.get()[pos] : nullptr;
	}

	/** @brief see if the cache can get a better search position to find an object
	 *  @param ref current object reference, must be valid
	 *  @param objID object we're looking for
	 *  @note Object IDs are sequential indices, so we'll try to get the start offset as close as possible
	 */
	void improve(ObjRef& ref, FWFS::Object::ID objID)
	{
		auto cachedId = objID - (objID % FWFS_CACHE_SPACING);
		if(cachedId <= ref.id) {
			return;
		}

		auto offsetPtr = getOffsetPtr(cachedId);
		if(offsetPtr != nullptr) {
#if FWFS_DEBUG
			debug_d("Cache hit #%u @ 0x%08X (for #%u)", cachedId, *offsetPtr, objID);
#endif
			ref.id = cachedId;
			ref.offset = *offsetPtr;
		}
	}

private:
	std::unique_ptr<uint32_t[]> offsets;
	FWFS::Object::ID offsetCount{0}; ///< Number of cached offsets
};

} // namespace FWFS
} // namespace IFS
