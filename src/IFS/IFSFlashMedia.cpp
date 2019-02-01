/*
 * FSFlashMedia.cpp
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 */

#include "IFSFlashMedia.h"
#include "IFSError.h"
#include "flashmem.h"
#include <algorithm>

/** @brief obtain the maximum size for an extent starting at a specified address in flash memory
 *  @param startAddress offset from start of flash
 *  @retval uint32_t maximum size
 *  @note if startAddress is outside valid flash range then 0 is returned
 */
static inline uint32_t getMaxSize(uint32_t startAddress)
{
	return (startAddress < (uint32_t)INTERNAL_FLASH_SIZE) ? (uint32_t)INTERNAL_FLASH_SIZE - startAddress : 0;
}

IFSFlashMedia::IFSFlashMedia(uint32_t startAddress, uint32_t size, FSMediaAttributes attr)
	: IFSMedia(std::min(size, getMaxSize(startAddress)), attr), _startAddress(startAddress)
{
}

IFSFlashMedia::IFSFlashMedia(uint32_t startAddress, FSMediaAttributes attr)
	: IFSMedia(getMaxSize(startAddress), attr), _startAddress(startAddress)
{
}

IFSFlashMedia::IFSFlashMedia(const void* startPtr, FSMediaAttributes attr)
	: IFSMedia(getMaxSize(flashmem_get_address(startPtr)), attr), _startAddress(flashmem_get_address(startPtr))
{
}

int IFSFlashMedia::setExtent(uint32_t size)
{
	if((_startAddress + size) > (uint32_t)INTERNAL_FLASH_SIZE)
		return FSERR_BadExtent;

	return IFSMedia::setExtent(size);
}

FSMediaInfo IFSFlashMedia::getinfo() const
{
	return FSMediaInfo{.type = eFMT_Flash, .bus = eBus_HSPI, .blockSize = INTERNAL_FLASH_SECTOR_SIZE};
}

int IFSFlashMedia::read(uint32_t offset, uint32_t size, void* buffer)
{
	//	m_printf("FSM(0x%08X, 0x%08X, %u, 0x%08X): ", _startAddress, offset, size, buffer);

	FS_CHECK_EXTENT(offset, size);
	uint32_t res = flashmem_read(buffer, _startAddress + offset, size);

	//	m_printf("%u\n", res);

	return (res == size) ? FS_OK : FSERR_ReadFailure;
}

int IFSFlashMedia::write(uint32_t offset, uint32_t size, const void* data)
{
	FS_CHECK_EXTENT(offset, size);
	FS_CHECK_WRITEABLE();

	uint32_t res = flashmem_write(data, _startAddress + offset, size);
	return (res == size) ? FS_OK : FSERR_WriteFailure;
}

int IFSFlashMedia::erase(uint32_t offset, uint32_t size)
{
	FS_CHECK_EXTENT(offset, size);
	FS_CHECK_WRITEABLE();

	uint32_t sect_first = flashmem_get_sector_of_address(_startAddress + offset);
	uint32_t sect_last = sect_first;
	while(sect_first <= sect_last) {
		if(!flashmem_erase_sector(sect_first++)) {
			return FSERR_EraseFailure;
		}
	}

	return FS_OK;
}
