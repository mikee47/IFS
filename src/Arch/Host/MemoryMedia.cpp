/*
 * MemoryMedia.cpp
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/MemoryMedia.h"
#include <IFS/Error.h>

namespace IFS
{
const uint32_t MAX_MEMORY_SIZE = 4096 * 1024U;
const uint32_t BLOCK_SIZE = 4096U;

MemoryMedia::MemoryMedia(const void* startPtr, Media::Attributes attr)
	: Media(MAX_MEMORY_SIZE, attr), startPtr(startPtr)
{
}

int MemoryMedia::setExtent(uint32_t size)
{
	if(size > MAX_MEMORY_SIZE) {
		return FSERR_BadExtent;
	}

	return Media::setExtent(size);
}

Media::Info MemoryMedia::getinfo() const
{
	return Media::Info{
		.type = Type::Flash,
		.bus = Bus::HSPI,
		.blockSize = BLOCK_SIZE,
	};
}

int MemoryMedia::read(uint32_t offset, uint32_t size, void* buffer)
{
	FS_CHECK_EXTENT(offset, size);

	memcpy(buffer, reinterpret_cast<const uint8_t*>(startPtr) + offset, size);

	return FS_OK;
}

int MemoryMedia::write(uint32_t offset, uint32_t size, const void* data)
{
	FS_CHECK_EXTENT(offset, size);
	FS_CHECK_WRITEABLE();

	memcpy(reinterpret_cast<uint8_t*>(const_cast<void*>(startPtr)) + offset, data, size);

	return FS_OK;
}

int MemoryMedia::erase(uint32_t offset, uint32_t size)
{
	FS_CHECK_EXTENT(offset, size);
	FS_CHECK_WRITEABLE();

	memset(reinterpret_cast<uint8_t*>(const_cast<void*>(startPtr)) + offset, 0xFF, size);

	return FS_OK;
}

} // namespace IFS
