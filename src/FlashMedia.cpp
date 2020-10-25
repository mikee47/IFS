/*
 * FSFlashMedia.cpp
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/FlashMedia.h"
#include "include/IFS/Error.h"
#include <esp_spi_flash.h>
#include <algorithm>

namespace IFS
{
/** @brief obtain the maximum size for an extent starting at a specified address in flash memory
 *  @param startAddress offset from start of flash
 *  @retval uint32_t maximum size
 *  @note if startAddress is outside valid flash range then 0 is returned
 */
static inline uint32_t getMaxSize(uint32_t startAddress)
{
	return (startAddress < (uint32_t)INTERNAL_FLASH_SIZE) ? (uint32_t)INTERNAL_FLASH_SIZE - startAddress : 0;
}

FlashMedia::FlashMedia(uint32_t startAddress, uint32_t size, FSMediaAttributes attr)
	: Media(std::min(size, getMaxSize(startAddress)), attr), m_startAddress(startAddress)
{
}

FlashMedia::FlashMedia(uint32_t startAddress, FSMediaAttributes attr)
	: Media(getMaxSize(startAddress), attr), m_startAddress(startAddress)
{
}

FlashMedia::FlashMedia(const void* startPtr, FSMediaAttributes attr)
	: Media(getMaxSize(flashmem_get_address(startPtr)), attr), m_startAddress(flashmem_get_address(startPtr))
{
}

int FlashMedia::setExtent(uint32_t size)
{
	if((m_startAddress + size) > (uint32_t)INTERNAL_FLASH_SIZE)
		return FSERR_BadExtent;

	return Media::setExtent(size);
}

FSMediaInfo FlashMedia::getinfo() const
{
	return FSMediaInfo{.type = eFMT_Flash, .bus = eBus_HSPI, .blockSize = INTERNAL_FLASH_SECTOR_SIZE};
}

int FlashMedia::read(uint32_t offset, uint32_t size, void* buffer)
{
	//	m_printf("FSM(0x%08X, 0x%08X, %u, 0x%08X): ", _startAddress, offset, size, buffer);

	FS_CHECK_EXTENT(offset, size);
	uint32_t res = flashmem_read(buffer, m_startAddress + offset, size);

	//	m_printf("%u\n", res);

	return (res == size) ? FS_OK : FSERR_ReadFailure;
}

int FlashMedia::write(uint32_t offset, uint32_t size, const void* data)
{
	FS_CHECK_EXTENT(offset, size);
	FS_CHECK_WRITEABLE();

	uint32_t res = flashmem_write(data, m_startAddress + offset, size);
	return (res == size) ? FS_OK : FSERR_WriteFailure;
}

int FlashMedia::erase(uint32_t offset, uint32_t size)
{
	FS_CHECK_EXTENT(offset, size);
	FS_CHECK_WRITEABLE();

	uint32_t sect_first = flashmem_get_sector_of_address(m_startAddress + offset);
	uint32_t sect_last = sect_first;
	while(sect_first <= sect_last) {
		if(!flashmem_erase_sector(sect_first++)) {
			return FSERR_EraseFailure;
		}
	}

	return FS_OK;
}

} // namespace IFS
