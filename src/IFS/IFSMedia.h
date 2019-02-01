/*
 * IFSMedia.h
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 *
 * Definitions to provide an abstraction for physical media which a filesystem is mounted on.
 */

#ifndef __IFS_MEDIA_H
#define __IFS_MEDIA_H

#include "IFSError.h"
#include "ifstypes.h"

/** @brief Physical media filing system is mounted on
 *  @note We'll use the term 'disk' when referring to physical media
 */
enum FSMediaType {
	eFMT_Unknown,
	eFMT_Flash,  ///< Flash memory (no wear levelling)
	eFMT_SDCard, ///< SD card - flash with wear levelling
	eFMT_Disk,   ///< Physical disk
};

/** @brief Transport mechanism for physical media
 *
 */
enum BusType {
	eBus_Unknown,
	eBus_System,   ///< System bus (e.g. memory controller)
	eBus_SDIO,	 ///< SD card interfaces
	eBus_SPI,	  ///< Serial Peripheral bus
	eBus_HSPI,	 ///< Hardware SPI
	eBus_I2C,	  ///< I2C bus
	eBus_Modbus,   ///< Modbus
	eBus_Ethernet, ///< Wired network
	eBus_WiFi,	 ///< Wireless network
};

enum FSMediaAttributes {
	eFMA_ReadWrite = 0x00,
	eFMA_ReadOnly = 0x01, ///< Prevent write/erase operations
};

struct FSMediaInfo {
	FSMediaType type;
	BusType bus;
	uint32_t blockSize; ///< Smallest allocation unit For erase
};

/** @brief defines an address range */
struct FSExtent {
	uint32_t start = 0;
	uint32_t length = 0;

	FSExtent()
	{
	}

	FSExtent(uint32_t start, uint32_t length)
	{
		this->start = start;
		this->length = length;
	}

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

// IFSMedia implementations can use this macro for standard extent check
#define FS_CHECK_EXTENT(_off, _sz)                                                                                     \
	{                                                                                                                  \
		uint32_t off = _off;                                                                                           \
		size_t sz = _sz;                                                                                               \
		if(!checkExtent(off, sz)) {                                                                                    \
			debug_e("%s(0x%08x, %u): Bad Extent, media size = 0x%08x", __PRETTY_FUNCTION__, off, sz, _size);           \
			assert(false);                                                                                             \
			return FSERR_BadExtent;                                                                                    \
		}                                                                                                              \
	}

#define FS_CHECK_WRITEABLE()                                                                                           \
	if(_attr & eFMA_ReadOnly) {                                                                                        \
		return FSERR_ReadOnly;                                                                                         \
	}

/** @brief virtual base class to access physical filesystem media
 *  @note this is typically flash memory, hence a separate erase method.
 *  All media is represented as a single valid contiguous extent, even if it
 *  is physically arranged differently, e.g. multiple regions spread across
 *  memory chips.
 */
class IFSMedia
{
public:
	IFSMedia(uint32_t size, FSMediaAttributes attr) : _size(size), _attr(attr)
	{
	}

	virtual ~IFSMedia()
	{
	}

	/** @brief called during filing system mount operation
	 *  @param size
	 *  @retval error code
	 *  @note Media objects are created with a limit on their size, but a filing system can use
	 *  this method to reduce that size. FWFS does this once it's determined its image size,
	 *  which provides an additional check that it doesn't exceed the maximum media size.
	 */
	virtual int setExtent(uint32_t size)
	{
		if(size > _size) {
			return FSERR_BadExtent;
		}

		_size = size;
		return FS_OK;
	}

	/** @brief get the size of this media in bytes */
	uint32_t mediaSize() const
	{
		return _size;
	}

	FSMediaAttributes attr() const
	{
		return _attr;
	}

	FSMediaType type() const
	{
		return getinfo().type;
	}

	BusType bus() const
	{
		return getinfo().bus;
	}

	uint32_t blockSize() const
	{
		return getinfo().blockSize;
	}

	/** @brief get some information about this media
	 *  @param info returned information
	 *  @retval error code
	 */
	virtual FSMediaInfo getinfo() const = 0;

	/** @brief read a block from disk
	 *  @param offset location to start reading
	 *  @param size bytes to read
	 *  @param buffer to store data
	 *  @retval error code
	 *  @note must fail if the request could not be fully satisfied
	 */
	virtual int read(uint32_t offset, uint32_t size, void* buffer) = 0;

	/** @brief write a block to disk
	 *  @param offset start location for write
	 *  @param size quantity of bytes to write
	 *  @param data to write
	 *  @retval error code
	 *  @note must fail if the request could not be fully satisfied
	 */
	virtual int write(uint32_t offset, uint32_t size, const void* data) = 0;

	/** @brief erase a block
	 *  @param offset start location for erase
	 *  @param size quantity of bytes to erase
	 *  @retval error code
	 *  @note must fail if the request could not be fully satisfied
	 */
	virtual int erase(uint32_t offset, uint32_t size) = 0;

	/** @brief read a name string into a buffer
	 *  @param buffer
	 *  @param bufsize space in buffer
	 *  @param offset
	 *  @param len number of characters in name
	 *  @retval error code, will return error if buffer too small
	 *  @note the string is always correctly terminated
	 */
	int readname(char* buffer, unsigned bufsize, uint32_t offset, unsigned len);

	/** @brief check whether the given extent is valid for this media */
	bool checkExtent(uint32_t offset, uint32_t size) const
	{
		return offset <= _size && (offset + size) <= _size;
	}

protected:
	uint32_t _size;			 ///< Size of media in bytes (always starts at 0)
	FSMediaAttributes _attr; ///< Specific media attributes
};

#endif // __IFS_MEDIA_H
