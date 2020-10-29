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

namespace IFS
{
#define IFS_MEDIA_TYPE_MAP(XX)                                                                                         \
	XX(Unknown, "Unknown")                                                                                             \
	XX(RAM, "RAM")                                                                                                     \
	XX(Flash, "Flash memory (no wear levelling)")                                                                      \
	XX(SDCard, "SD card - flash with wear levelling")                                                                  \
	XX(Disk, "Physical disk")

#define IFS_MEDIA_BUS_MAP(XX)                                                                                          \
	XX(Unknown, "Unknown")                                                                                             \
	XX(System, "System bus (e.g. memory controller)")                                                                  \
	XX(SDIO, "SD card interfaces")                                                                                     \
	XX(SPI, "Serial Peripheral bus")                                                                                   \
	XX(I2C, "I2C bus")                                                                                                 \
	XX(Modbus, "Modbus")                                                                                               \
	XX(Ethernet, "Wired network")                                                                                      \
	XX(WiFi, "Wireless network")

#define IFS_MEDIA_ATTRIBUTE_MAP(XX) XX(ReadOnly, "Prevent write/erase operations")

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

#define FS_CHECK_WRITEABLE()                                                                                           \
	if(m_attr[Attribute::ReadOnly]) {                                                                                  \
		return Error::ReadOnly;                                                                                        \
	}

/** @brief virtual base class to access physical filesystem media
 *  @note this is typically flash memory, hence a separate erase method.
 *  All media is represented as a single valid contiguous extent, even if it
 *  is physically arranged differently, e.g. multiple regions spread across
 *  memory chips.
 */
class Media
{
public:
	/**
	 * @brief Physical media filing system is mounted on
	 * @note We'll use the term 'disk' when referring to physical media
	 */
	enum class Type {
#define XX(tag, comment) tag,
		IFS_MEDIA_TYPE_MAP(XX)
#undef XX
	};

	/**
	 * @brief Transport mechanism for physical media
	 */
	enum class Bus {
#define XX(tag, comment) tag,
		IFS_MEDIA_BUS_MAP(XX)
#undef XX
	};

	enum class Attribute {
#define XX(tag, comment) tag,
		IFS_MEDIA_ATTRIBUTE_MAP(XX)
#undef XX
			MAX
	};

	using Attributes = BitSet<uint8_t, Attribute, size_t(Attribute::MAX)>;
	static constexpr Attributes ReadWrite{};
	static constexpr Attributes ReadOnly{Attribute::ReadOnly};

	struct Info {
		Type type;
		Bus bus;
		uint32_t blockSize; ///< Smallest allocation unit For erase
	};

	Media(uint32_t size, Attributes attr) : m_size(size), m_attr(attr)
	{
	}

	virtual ~Media()
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
		if(size > m_size) {
			return Error::BadExtent;
		}

		m_size = size;
		return FS_OK;
	}

	/** @brief get the size of this media in bytes */
	uint32_t mediaSize() const
	{
		return m_size;
	}

	Attributes attr() const
	{
		return m_attr;
	}

	Type type() const
	{
		return getinfo().type;
	}

	Bus bus() const
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
	virtual Info getinfo() const = 0;

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
		return offset <= m_size && (offset + size) <= m_size;
	}

protected:
	uint32_t m_size;   ///< Size of media in bytes (always starts at 0)
	Attributes m_attr; ///< Specific media attributes
};

} // namespace IFS

String toString(IFS::Media::Type type);
String toString(IFS::Media::Bus bus);
String toString(IFS::Media::Attribute attr);
