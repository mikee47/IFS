/**
 * FileSystem.h
 *
 * Created August 2018 by mikee471
 *
 * Copyright 2019 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the IFS Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ****/

/**	@defgroup filesystem File system
 *	@brief	Installable file system
 *  @{
 */

#pragma once

#include "IFileSystem.h"
#include <Delegate.h>

namespace IFS
{
/**
 * @brief Installable File System base class
 * 
 * Adds additional methods to ease use over base IFileSystem.
 */
class FileSystem : public IFileSystem
{
public:
	static constexpr FileSystem& cast(IFileSystem& fs)
	{
		return static_cast<FileSystem&>(fs);
	}

	static constexpr FileSystem* cast(IFileSystem* fs)
	{
		return static_cast<FileSystem*>(fs);
	}

	using IFileSystem::opendir;
	/**
	 * @brief open a directory for reading
     */
	int opendir(const String& path, DirHandle& dir)
	{
		return opendir(path.c_str(), dir);
	}

	using IFileSystem::mkdir;

	int mkdir(const String& path)
	{
		return mkdir(path.c_str());
	}

	/**
	 * @brief Create a directory and any intermediate directories if they do not already exist
	 * @param path Path to directory. If no trailing '/' is present the final element is considered a filename.
	 * @retval int error code
	 */
	int makedirs(const char* path);

	int makedirs(const String& path)
	{
		return makedirs(path.c_str());
	}

	using IFileSystem::stat;
	int stat(const String& path, Stat* s)
	{
		return stat(path.c_str(), s);
	}
	int stat(const String& path, Stat& s)
	{
		return stat(path.c_str(), &s);
	}

	using IFileSystem::fstat;
	int fstat(FileHandle file, Stat& stat)
	{
		return fstat(file, &stat);
	}

	using IFileSystem::open;
	FileHandle open(const String& path, OpenFlags flags)
	{
		return open(path.c_str(), flags);
	}

	using IFileSystem::ftruncate;
	int ftruncate(FileHandle file)
	{
		int pos = tell(file);
		return (pos < 0) ? pos : ftruncate(file, pos);
	}

	/**
	 * @brief Truncate a file to a specific size
	 * @param fileName File to truncate
     * @retval int new file size, or error code
	 */
	int truncate(const char* fileName, size_t newSize);

	int truncate(const String& fileName, size_t newSize)
	{
		return truncate(fileName.c_str(), newSize);
	}

	using IFileSystem::rename;
	/**
	 * @name rename a file
     * @param oldpath
     * @param newpath
     * @retval int error code
     * @{
     */
	int rename(const String& oldpath, const String& newpath)
	{
		return rename(oldpath.c_str(), newpath.c_str());
	}

	using IFileSystem::remove;
	/**
	 * @brief remove (delete) a file by path
     * @param path
     * @retval int error code
     */
	int remove(const String& path)
	{
		return remove(path.c_str());
	}

	int setAttribute(FileHandle file, AttributeTag tag, const void* data, size_t size)
	{
		return fsetxattr(file, tag, data, size);
	}

	int setAttribute(const char* file, AttributeTag tag, const void* data, size_t size)
	{
		return setxattr(file, tag, data, size);
	}

	int setAttribute(const String& file, AttributeTag tag, const void* data, size_t size)
	{
		return setxattr(file.c_str(), tag, data, size);
	}

	template <typename T> int setAttribute(const T& file, AttributeTag tag, const String& data)
	{
		return setAttribute(file, tag, data.c_str(), data.length());
	}

	int getAttribute(FileHandle file, AttributeTag tag, void* buffer, size_t size)
	{
		return fgetxattr(file, tag, buffer, size);
	}

	int getAttribute(const char* file, AttributeTag tag, void* buffer, size_t size)
	{
		return getxattr(file, tag, buffer, size);
	}

	int getAttribute(const String& file, AttributeTag tag, void* buffer, size_t size)
	{
		return getxattr(file.c_str(), tag, buffer, size);
	}

	template <typename T> int getAttribute(const T& file, AttributeTag tag, String& value)
	{
		char buffer[256];
		int len = getAttribute(file, tag, buffer, sizeof(buffer));
		if(len != Error::BufferTooSmall) {
			if(len >= 0) {
				value.setString(buffer, len);
				if(!value) {
					return Error::NoMem;
				}
			}
			return len;
		}
		if(!value.setLength(len)) {
			return Error::NoMem;
		}
		return getAttribute(file, tag, value.begin(), value.length());
	}

	template <typename T> String getAttribute(const T& file, AttributeTag tag)
	{
		String value;
		int err = getAttribute(file, tag, value);
		return (err < 0) ? nullptr : value;
	}

	template <typename T> int removeAttribute(const T& file, AttributeTag tag)
	{
		return setAttribute(file, tag, nullptr, 0);
	}

	template <typename T, typename... ParamTypes>
	int setUserAttribute(const T& file, uint8_t tagValue, ParamTypes... params)
	{
		return setAttribute(file, getUserAttributeTag(tagValue), params...);
	}

	template <typename T, typename... ParamTypes>
	int getUserAttribute(const T& file, uint8_t tagValue, ParamTypes... params)
	{
		return getAttribute(file, getUserAttributeTag(tagValue), params...);
	}

	template <typename T> String getUserAttribute(const T& file, uint8_t tagValue)
	{
		return getAttribute(file, getUserAttributeTag(tagValue));
	}

	template <typename T> bool removeUserAttribute(const T& file, uint8_t tagValue)
	{
		return removeAttribute(file, getUserAttributeTag(tagValue));
	}

	/**
	 * @brief Set access control information for file
     * @param file handle or path to file
     * @param acl
     * @retval int error code
     */
	template <typename T> int setacl(const T& file, const ACL& acl)
	{
		return setAttribute(file, AttributeTag::Acl, &acl, sizeof(acl));
	}

	/**
	 * @brief Set file attributes
     * @param file handle or path to file
     * @param attr
     * @retval int error code
     */
	template <typename T> int setattr(const T& file, FileAttributes attr)
	{
		return setAttribute(file, AttributeTag::FileAttributes, &attr, sizeof(attr));
	}

	/**
	 * @brief Set modification time for file
     * @param file handle or path to file
     * @retval int error code
     * @note any subsequent writes to file will reset this to current time
     */
	template <typename T> int settime(const T& file, time_t mtime)
	{
		TimeStamp ts;
		ts = mtime;
		return setAttribute(file, AttributeTag::ModifiedTime, &ts, sizeof(ts));
	}

	/**
	 * @brief Set file compression information
	 * @param file
     * @param compression
     * @retval int error code
     */
	template <typename T> int setcompression(const T& file, const Compression& compression)
	{
		return setAttribute(file, AttributeTag::Compression, &compression, sizeof(compression));
	}

	/** @brief  Get size of file
	 *  @param  file File handle
	 *  @retval uint32_t Size of file in bytes, 0 on error
	 */
	uint32_t getSize(FileHandle file);

	/** @brief  Get size of file
	 *  @param  fileName Name of file
	 *  @retval uint32_t Size of file in bytes, 0 on error
	 */
	uint32_t getSize(const char* fileName);

	uint32_t getSize(const String& fileName)
	{
		return getSize(fileName.c_str());
	}

	/**
	 * @brief Callback for readContent method
	 * @param buffer
	 * @param size
	 * @retval int Return number of bytes consumed, < size to stop
	 * If < 0 then this is returned as error code to `readContent` call.
	 */
	using ReadContentCallback = Delegate<int(const char* buffer, size_t size)>;

	/**
	 * @brief Read from current file position and invoke callback for each block read
	 * @param file
	 * @param size Maximum number of bytes to read
	 * @param callback
	 * @retval int Number of bytes processed, or error code
	 */
	int readContent(FileHandle file, size_t size, ReadContentCallback callback);

	/**
	 * @brief Read from current file position to end of file and invoke callback for each block read
	 * @param file
	 * @param callback
	 * @retval int Number of bytes processed, or error code
	 */
	int readContent(FileHandle file, ReadContentCallback callback);

	/**
	 * @brief Read entire file content in blocks, invoking callback after every read
	 * @param filename
	 * @param callback
	 * @retval int Number of bytes processed, or error code
	 */
	int readContent(const String& filename, ReadContentCallback callback);

	/**
	 * @name  Read content of a file
	 * @param  fileName Name of file to read from
	 * @param  buffer Pointer to a character buffer in to which to read the file content
	 * @param  bufSize Quantity of bytes to read from file
	 * @retval size_t Quantity of bytes read from file or zero on failure
	 *
	 * After calling this function the content of the file is placed in to a c-string
	 * Ensure there is sufficient space in the buffer for file content
	 * plus extra trailing null, i.e. at least bufSize + 1
	 * Always check the return value!
	 *
	 * Returns 0 if the file could not be read
	 * 
	 * @{
	 */
	size_t getContent(const char* fileName, char* buffer, size_t bufSize);

	size_t getContent(const String& fileName, char* buffer, size_t bufSize)
	{
		return getContent(fileName.c_str(), buffer, bufSize);
	}

	/** @} */

	/**
	 * @brief  Read content of a file
	 * @param  fileName Name of file to read from
	 * @retval String String variable in to which to read the file content
	 * @note   After calling this function the content of the file is placed in to a string.
	 * The result will be an invalid String (equates to `false`) if the file could not be read.
	 * If the file exists, but is empty, the result will be an empty string "".
	 */
	String getContent(const String& fileName);

	/**
	 * @name  Create or replace file with defined content
	 * @param  fileName Name of file to create or replace
	 * @param  content Pointer to c-string containing content to populate file with
	 * @retval int Number of bytes transferred or error code
	 * @param  length (optional) number of characters to write
	 *
	 * This function creates a new file or replaces an existing file and
	 * populates the file with the content of a c-string buffer.
	 * 
	 * @{
	 */
	int setContent(const char* fileName, const void* content, size_t length);

	int setContent(const char* fileName, const char* content)
	{
		return setContent(fileName, content, (content == nullptr) ? 0 : strlen(content));
	}

	int setContent(const String& fileName, const char* content)
	{
		return setContent(fileName.c_str(), content);
	}

	int setContent(const String& fileName, const void* content, size_t length)
	{
		return setContent(fileName.c_str(), content, length);
	}

	int setContent(const String& fileName, const String& content)
	{
		return setContent(fileName.c_str(), content.c_str(), content.length());
	}

	/** @} */
};

#ifdef ARCH_HOST
namespace Host
{
FileSystem& getFileSystem();
}
#endif

} // namespace IFS

/** @} */
