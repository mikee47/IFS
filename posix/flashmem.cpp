/*
 * flashmem.cpp
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 */

#include "flashmem.h"
#include <assert.h>
#include <string.h>
#include "IFS/ifstypes.h"

static uint8_t g_flashmem[INTERNAL_FLASH_SIZE];

#define CHECK_ALIGNMENT(_x) assert((reinterpret_cast<uint32_t>(_x) & 0x00000003) == 0)

#define CHECK_RANGE(_addr, _size) { \
	uint32_t __addr = _addr; \
	uint32_t __size = _size; \
	if(__addr + __size > INTERNAL_FLASH_SIZE) { \
		debug_e("Out of range: 0x%08x, %u", __addr, __size); \
		assert(false); \
	} \
}

uint32_t flashmem_get_size_bytes()
{
	return INTERNAL_FLASH_SIZE;
}

uint16_t flashmem_get_size_sectors()
{
	return INTERNAL_FLASH_SIZE / INTERNAL_FLASH_SECTOR_SIZE;
}

uint32_t flashmem_write_internal(const void* from, uint32_t toaddr, uint32_t size)
{
	CHECK_ALIGNMENT(toaddr);
	CHECK_ALIGNMENT(size);
	CHECK_RANGE(toaddr, size);
	memcpy(&g_flashmem[toaddr], from, size);
	return size;
}

uint32_t flashmem_read_internal(void* to, uint32_t fromaddr, uint32_t size)
{
	CHECK_ALIGNMENT(fromaddr);
	CHECK_ALIGNMENT(size);
	CHECK_RANGE(fromaddr, size);
	memcpy(to, &g_flashmem[fromaddr], size);
	return size;
}

uint32_t flashmem_write(const void* from, uint32_t toaddr, uint32_t size)
{
	CHECK_RANGE(toaddr, size);
	memcpy(&g_flashmem[toaddr], from, size);
	return size;
}

uint32_t flashmem_read(void* to, uint32_t fromaddr, uint32_t size)
{
	CHECK_RANGE(fromaddr, size);
	memcpy(to, &g_flashmem[fromaddr], size);
	return size;
}

uint32_t flashmem_get_address(const void* memptr)
{
	return (uint32_t)memptr - INTERNAL_FLASH_START_ADDRESS;
}

uint32_t flashmem_get_sector_of_address(uint32_t addr)
{
	return addr / INTERNAL_FLASH_SECTOR_SIZE;
}

bool flashmem_erase_sector(uint32_t sector_id)
{
	uint32_t addr = sector_id * INTERNAL_FLASH_SECTOR_SIZE;
	CHECK_RANGE(addr, INTERNAL_FLASH_SECTOR_SIZE);
	memset(&g_flashmem[addr], 0xFF, INTERNAL_FLASH_SECTOR_SIZE);
	return true;
}

