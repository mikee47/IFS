/*
 * FWObjRefCache.h
 *
 *  Created on: 1 Sep 2018
 *      Author: mikee47
 */

#ifndef __FWOBJREF_CACHE_H
#define __FWOBJREF_CACHE_H

#include "IFSObjectStore.h"

#define FWFS_OBJECT_CACHE

// Add a cache entry at regular index positions - pick a number here which is a modulus of 2
#define FWFS_CACHE_SPACING 8
#define FWFS_CACHE_MASK (FWFS_CACHE_SPACING - 1)

#ifdef FWFS_OBJECT_CACHE

/** @brief Cache the locations of several objects to improve search speed
 */
class FWObjRefCache
{
public:
	~FWObjRefCache()
	{
		clear();
	}

	void initialise(FWObjectID objectCount)
	{
		clear();
		if(objectCount < FWFS_CACHE_SPACING) {
			return;
		}
		_offsetCount = (objectCount / FWFS_CACHE_SPACING) - 1;
		_offsets = new uint32_t[_offsetCount];
		if(_offsets == nullptr) {
			_offsetCount = 0;
		}
	}

	void clear()
	{
		_offsetCount = 0;
		delete[] _offsets;
		_offsets = nullptr;
	}

	void add(const FWObjRef& ref)
	{
		if(ref.id && (ref.id & FWFS_CACHE_MASK) == 0) {
			auto po = getOffset(ref.id);
			if(po != 0) {
				debug_d("Caching #%u @ 0x%08X", ref.id, ref.offset);
				*po = ref.offset;
			}
		}
	}

	uint32_t* getOffset(FWObjectID objIndex)
	{
		int pos = (objIndex / FWFS_CACHE_SPACING) - 1;
		return (pos >= 0 && pos < _offsetCount) ? &_offsets[pos] : nullptr;
	}

	/** @brief see if the cache can get a better search position to find an object
	 *  @param ref current object reference, must be valid
	 *  @param objID object we're looking for
	 *  @note FWRO object IDs are sequential indices, so we'll try to get the start offset as close as possible
	 */
	void improve(FWObjRef& ref, FWObjectID objID)
	{
		FWObjectID cachedIndex = objID & ~FWFS_CACHE_MASK;
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
	uint32_t* _offsets = nullptr;
	FWObjectID _offsetCount = 0; ///< Number of cached offsets
};

#endif

#endif // __FWOBJREF_CACHE_H
