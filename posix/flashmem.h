#ifndef __FLASHMEM_H
#define __FLASHMEM_H

#include <stdint.h>

#define INTERNAL_FLASH_SECTOR_SIZE 0x1000
#define INTERNAL_FLASH_START_ADDRESS 0x40200000
#define INTERNAL_FLASH_SIZE 0x400000

uint32_t flashmem_write(const void* from, uint32_t toaddr, uint32_t size);
uint32_t flashmem_read(void* to, uint32_t fromaddr, uint32_t size);
bool flashmem_erase_sector(uint32_t sector_id);
uint32_t flashmem_get_address(const void* memptr);
uint32_t flashmem_get_sector_of_address(uint32_t addr);

#endif // __FLASHMEM_H
