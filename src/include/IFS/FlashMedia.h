/*
 * FlashMedia.h
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 *
 * Implementation of IFS media layer on flash memory.
 *
 */

#pragma once

#include "Media.h"

namespace IFS
{
/** @brief Media object representing storage area of flash memory.
 *  @note This uses the system SPI flash read/write routines and restricts all accesses
 *  to a specific memory region. If created with read-only attribute will also prevent
 *  writes or erasures.
 */
class FlashMedia : public Media
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
	FlashMedia(uint32_t startAddress, uint32_t size, Media::Attributes attr);

	/** @brief constructor to calculate extent from startAddress
	 *  @param startAddress
	 *  @param attr
	 */
	FlashMedia(uint32_t startAddress, Media::Attributes attr);

	/** @brief constructor to calculate extent from a memory pointer
	 *  @param startPtr must be in flash memory
	 *  @param attr
	 */
	FlashMedia(const void* startPtr, Media::Attributes attr);

	int setExtent(uint32_t size) override;
	Media::Info getinfo() const override;
	int read(uint32_t offset, uint32_t size, void* buffer) override;
	int write(uint32_t offset, uint32_t size, const void* data) override;
	int erase(uint32_t offset, uint32_t size) override;

private:
	uint32_t m_startAddress;
};

} // namespace IFS
