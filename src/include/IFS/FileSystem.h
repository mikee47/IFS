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
	/**
	 * @brief get file information
     */
	int stat(const String& path, Stat* s)
	{
		return stat(path.c_str(), s);
	}
	int stat(const String& path, Stat& s)
	{
		return stat(path.c_str(), &s);
	}

	using IFileSystem::fstat;
	/**
	 * @brief get file information
     */
	int fstat(FileHandle file, Stat& stat)
	{
		return fstat(file, &stat);
	}

	using IFileSystem::open;
	/**
	 * @brief open a file by name/path
     * @param path full path to file
     * @param flags opens for opening file
     * @retval FileHandle file handle or error code
     * @{
     */
	FileHandle open(const String& path, OpenFlags flags)
	{
		return open(path.c_str(), flags);
	}

	using IFileSystem::ftruncate;
	/**
	 * @brief Truncate an open file at the current cursor position
	 */
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

	using IFileSystem::setxattr;
	int setxattr(const String& path, AttributeTag tag, const void* data, size_t size)
	{
		return setxattr(path.c_str(), tag, data, size);
	}

	using IFileSystem::getxattr;
	int getxattr(const String& path, AttributeTag tag, void* buffer, size_t size)
	{
		return getxattr(path.c_str(), tag, buffer, size);
	}

	int fremovexattr(FileHandle file, AttributeTag tag)
	{
		return fsetxattr(file, tag, nullptr, 0);
	}

	template <typename T> int removexattr(const T& path, AttributeTag tag)
	{
		return setxattr(path, tag, nullptr, 0);
	}

	/**
	 * @brief Set access control information for file
     * @param file handle to open file
     * @param acl
     * @retval int error code
     */
	int setacl(FileHandle file, const ACL& acl)
	{
		return fsetxattr(file, AttributeTag::Acl, &acl, sizeof(acl));
	}

	int setacl(const char* path, const ACL& acl)
	{
		return setxattr(path, AttributeTag::Acl, &acl, sizeof(acl));
	}

	/**
	 * @brief Set file attributes
     * @param path Full path to file
     * @param attr
     * @retval int error code
     */
	template <typename T> int setattr(const T& path, FileAttributes attr)
	{
		return setxattr(path, AttributeTag::FileAttributes, &attr, sizeof(attr));
	}

	/**
	 * @brief Set modification time for file
     * @param file handle to open file, must have write access
     * @retval int error code
     * @note any subsequent writes to file will reset this to current time
     */
	int settime(FileHandle file, time_t mtime)
	{
		TimeStamp ts;
		ts = mtime;
		return fsetxattr(file, AttributeTag::ModifiedTime, &ts, sizeof(ts));
	}

	/**
	 * @brief Set modification time for file or directory
     * @param path Full path to file or directory
     * @retval int error code
     */
	template <typename T> int settime(const T& path, time_t mtime)
	{
		TimeStamp ts;
		ts = mtime;
		return setxattr(path, AttributeTag::ModifiedTime, &ts, sizeof(ts));
	}

	/**
	 * @brief Set file compression information
	 * @param file
     * @param compression
     * @retval int error code
     */
	int setcompression(FileHandle file, const Compression& compression)
	{
		return fsetxattr(file, AttributeTag::Compression, &compression, sizeof(compression));
	}

	template <typename T> int setcompression(const T& path, const Compression& compression)
	{
		return setxattr(path, AttributeTag::Compression, &compression, sizeof(compression));
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
