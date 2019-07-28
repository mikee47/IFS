/*
 * IFSMemoryMedia.h
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 *
 * Implementation of IFS media layer on flash memory.
 *
 */

#pragma once

#include <IFS/IFSMedia.h>

/** @brief Media object representing storage in regular memory.
 *  @note If using IMPORT_FSTR to place filesystem images into flash, they cannot
 *  be accessed using the Flash API because the Host emulates flash memory using
 *  a separate file.
 *  Note that IMPORT_FSTR will consume program memory, which is limited to 1MB.
 */
class IFSMemoryMedia : public IFSMedia
{
public:
	/** @brief constructor to calculate extent from a memory pointer
	 *  @param startPtr must be in flash memory
	 *  @param attr
	 */
	IFSMemoryMedia(const void* startPtr, FSMediaAttributes attr);

	int setExtent(uint32_t size) override;
	FSMediaInfo getinfo() const override;
	int read(uint32_t offset, uint32_t size, void* buffer) override;
	int write(uint32_t offset, uint32_t size, const void* data) override;
	int erase(uint32_t offset, uint32_t size) override;

private:
	const void* startPtr;
};
