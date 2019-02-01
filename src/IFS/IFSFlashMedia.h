/*
 * IFSFlashMedia.h
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 *
 * Implementation of IFS media layer on flash memory.
 *
 */

#ifndef __IFS_FLASH_MEDIA_H
#define __IFS_FLASH_MEDIA_H

#include "IFSMedia.h"

/** @brief Media object representing storage area of flash memory.
 *  @note This uses the system SPI flash read/write routines and restricts all accesses
 *  to a specific memory region. If created with read-only attribute will also prevent
 *  writes or erasures.
 */
class IFSFlashMedia : public IFSMedia
{
public:
	/** @brief standard constructor
	 *  @param startAddress expressed as offset from start of flash memory
	 *  @param size maximum extent for this media object
	 *  @param attr
	 *  @note Filing system may further restrict extent using setExtent()
	 *  If the specified extent (startAddress + size) exceeds valid flash
	 *  memory range then size is adjusted downwards. If startAddress is invalid
	 *  then size will be set to 0 and any memory access requests will fail.
	 */
	IFSFlashMedia(uint32_t startAddress, uint32_t size, FSMediaAttributes attr);

	/** @brief constructor to calculate extent from startAddress
	 *  @param startAddress
	 *  @param attr
	 */
	IFSFlashMedia(uint32_t startAddress, FSMediaAttributes attr);

	/** @brief constructor to calculate extent from a memory pointer
	 *  @param startPtr must be in flash memory
	 *  @param attr
	 */
	IFSFlashMedia(const void* startPtr, FSMediaAttributes attr);

	int setExtent(uint32_t size) override;
	FSMediaInfo getinfo() const override;
	int read(uint32_t offset, uint32_t size, void* buffer) override;
	int write(uint32_t offset, uint32_t size, const void* data) override;
	int erase(uint32_t offset, uint32_t size) override;

private:
	uint32_t _startAddress;
};

#endif // __IFS_FLASH_MEDIA_H
