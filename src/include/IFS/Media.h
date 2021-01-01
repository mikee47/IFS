/*
 * Media.h
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 *
 * Definitions to provide an abstraction for physical media which a filesystem is mounted on.
 */

#pragma once

#include "Error.h"
#include "Types.h"
#include <Storage/Partition.h>

namespace IFS
{
/** @brief defines an address range */
struct Extent {
	uint32_t start{0};
	uint32_t length{0};

	// Last valid address in this extent
	uint32_t end() const
	{
		return start + length - 1;
	}

	bool contains(uint32_t address) const
	{
		return address >= start && address <= end();
	}
};

// Media implementations can use this macro for standard extent check
#define FS_CHECK_EXTENT(offset, size)                                                                                  \
	{                                                                                                                  \
		uint32_t off = offset;                                                                                         \
		size_t sz = size;                                                                                              \
		if(!checkExtent(off, sz)) {                                                                                    \
			debug_e("%s(0x%08x, %u): Bad Extent, media size = 0x%08x", __PRETTY_FUNCTION__, off, sz, m_size);          \
			assert(false);                                                                                             \
			return Error::BadExtent;                                                                                   \
		}                                                                                                              \
	}

} // namespace IFS
