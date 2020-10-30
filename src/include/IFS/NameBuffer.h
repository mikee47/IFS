/*
 * NameBuffer.h
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#pragma once

#include "Types.h"
#include "Error.h"

namespace IFS
{
/**
 * @brief defines a 'safe' name buffer
 * @note Instead of including a fixed name array in FileStat (and IFileSystem::Info) structures,
 * we use a NameBuffer to identify a separate buffer. This has several advantages:
 *
 *		- Maximum size is not fixed
 *  	- Finding and copying the name is optional
 *  	- Actual name length is returned in the 'length' field, regardless of size
 *  	- A NameBuffer structure (or one containing it) only requires initialising once before
 *  	a loop operation as buffer/size are preserved.
 *
 * There are fancier ways to do this but a structure is transparent and requires no heap allocation.
 *
 * @note `length` always reflects the _required_ name/path length, and may be longer than size.
 */
struct NameBuffer {
	char* buffer{nullptr}; ///< Buffer to store name
	uint16_t size{0};	  ///< IN: Size of buffer
	uint16_t length{0};	///< OUT: length of name

	NameBuffer()
	{
	}

	NameBuffer(char* buffer, uint16_t size, uint16_t length = 0) : buffer(buffer), size(size), length(length)
	{
	}

	/**
	 * @brief Make a NameBuffer point to contents of a String
	 */
	NameBuffer(String& s) : buffer(s.begin()), size(s.length()), length(s.length())
	{
	}

	operator const char*() const
	{
		return buffer;
	}

	/**
	 * @brief copies text from a source buffer into a name buffer
	 * @param src source name
	 * @param srclen number of characters in name
	 * @note length field is always set to srclen, regardless of number of characters copied.
	 */
	int copy(const char* src, uint16_t srclen)
	{
		length = srclen;
		uint16_t copylen = std::min(srclen, size);
		if(copylen != 0) {
			memcpy(buffer, src, copylen);
		}
		terminate();
		return (copylen == srclen) ? FS_OK : Error::BufferTooSmall;
	}

	int copy(const char* src)
	{
		return copy(src, strlen(src));
	}

	int copy(const NameBuffer& name)
	{
		return copy(name.buffer, name.length);
	}

	/**
	 * @brief When building file paths this method simplified appending separators
	 * @retval int error code
	 * @note if the path is not empty, a separator character is appended
	 */
	int addSep()
	{
		// No separator required if path is empty
		if(length == 0) {
			return FS_OK;
		}

		if(length + 1 < size) {
			buffer[length++] = '/';
			buffer[length] = '\0';
			return FS_OK;
		}

		++length;
		return Error::BufferTooSmall;
	}

	/**
	 * @brief get a pointer to the next write position
	 * @retval char*
	 * @note use space() to ensure buffer doesn't overrun
	 * When writing text be sure to call terminate() when complete
	 */
	char* endptr()
	{
		return buffer + length;
	}

	/**
	 * @brief get the number of free characters available
	 * @retval uint16_t
	 * @note returns 0 if buffer has overrun
	 */
	uint16_t space()
	{
		return (length < size) ? size - length : 0;
	}

	/**
	 * @brief ensure the buffer has a nul terminator, even if it means overwriting content
	 */
	void terminate()
	{
		if(length < size) {
			buffer[length] = '\0';
		} else if(size != 0) {
			buffer[size - 1] = '\0';
		}
	}

	/**
	 * @brief determine if name buffer overflowed
	 * @note Compares returned length with buffer size; A nul terminator is always appended, so size should be >= (length + 1)
	 */
	bool overflow() const
	{
		return length >= size;
	}
};

/**
 * @brief a quick'n'dirty name buffer with maximum path allocation
 */
struct FileNameBuffer : public NameBuffer {
public:
	FileNameBuffer() : NameBuffer(buffer, sizeof(buffer))
	{
	}

private:
	// maximum size for a byte-length string, + 1 char for nul terminator
	char buffer[256];
};

} // namespace IFS
