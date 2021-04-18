/**
 * File.h
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

#pragma once

#include "FsBase.h"

namespace IFS
{
/**
 * @brief  Wraps up all file access methods
 */
class File : public FsBase
{
public:
	using FsBase::FsBase;

	// Common flag combinations
	static constexpr OpenFlags ReadOnly{OpenFlag::Read};
	static constexpr OpenFlags WriteOnly{OpenFlag::Write};
	static constexpr OpenFlags ReadWrite{OpenFlag::Read | OpenFlag::Write};
	static constexpr OpenFlags Create{OpenFlag::Create};
	static constexpr OpenFlags Append{OpenFlag::Append};
	static constexpr OpenFlags Truncate{OpenFlag::Truncate};
	static constexpr OpenFlags CreateNewAlways{OpenFlag::Create | OpenFlag::Truncate};

	~File()
	{
		close();
	}

	explicit operator bool() const
	{
		return handle >= 0;
	}

	/**
	 * @brief get file information
     * @param stat structure to return information in, may be null
     * @retval bool true on success
     */
	bool stat(Stat& stat)
	{
		GET_FS(false);
		return check(fs->fstat(handle, &stat));
	}

	/**
	 * @brief Low-level and non-standard file control operations
	 * @param code FCNTL_XXX code
	 * @param buffer Input/Output buffer
	 * @param bufSize Size of buffer
	 * @retval int error code or, on success, data size
	 * 
	 * To simplify usage the same buffer is used for both input and output.
	 * Only the size of the buffer is provided. If a specific FCNTL code requires more
	 * information then it will be contained within the provided data.
	 */
	int control(ControlCode code, void* buffer, size_t bufSize)
	{
		GET_FS(lastError);
		int res = fs->fcontrol(handle, code, buffer, bufSize);
		check(res);
		return res;
	}

	/**
	 * @brief open a file by name/path
     * @param path full path to file
     * @param flags opens for opening file
     * @retval bool true on success
     */
	bool open(const String& path, OpenFlags flags = OpenFlag::Read)
	{
		GET_FS(false);
		fs->close(handle);
		handle = fs->open(path.c_str(), flags);
		return check(handle);
	}

	/**
	 * @brief open a file from it's stat structure
     * @param stat obtained from readdir()
     * @param flags opens for opening file
     * @retval bool true on success
     */
	bool open(const Stat& stat, OpenFlags flags = OpenFlag::Read)
	{
		GET_FS_BOOL();
		fs->close(handle);
		handle = fs->fopen(stat, flags);
		return check(handle);
	}

	/**
	 * @brief close an open file
     * @retval bool true on success
     */
	bool close()
	{
		if(handle < 0) {
			return true;
		}
		GET_FS(false);
		int res = fs->close(handle);
		handle = -1;
		return check(res);
	}

	/**
	 * @brief read content from a file and advance cursor
     * @param data buffer to write into
     * @param size size of file buffer, maximum number of bytes to read
     * @retval int number of bytes read or error code
     */
	int read(void* data, size_t size)
	{
		GET_FS(lastError);
		int res = fs->read(handle, data, size);
		check(res);
		return res;
	}

	/**
	 * @brief write content to a file at current position and advance cursor
     * @param data buffer to read from
     * @param size number of bytes to write
     * @retval int number of bytes written or error code
     */
	int write(const void* data, size_t size)
	{
		GET_FS(lastError);
		int res = fs->write(handle, data, size);
		check(res);
		return res;
	}

	bool write(const String& s)
	{
		size_t len = s.length();
		return write(s.c_str(), len) == int(len);
	}

	/**
	 * @brief change file read/write position
     * @param offset position relative to origin
     * @param origin where to seek from (start/end or current position)
     * @retval int current position or error code
     */
	int seek(int offset, SeekOrigin origin)
	{
		GET_FS(lastError);
		int res = fs->lseek(handle, offset, origin);
		check(res);
		return res;
	}

	/**
	 * @brief determine if current file position is at end of file
     * @retval bool true if at EOF or file is invalid
     */
	bool eof()
	{
		auto fs = getFileSystem();
		if(fs == nullptr) {
			return true;
		}
		int res = fs->eof(handle);
		check(res);
		return res != FS_OK;
	}

	/**
	 * @brief get current file position
     * @retval int32_t current position relative to start of file, or error code
     */
	int32_t tell()
	{
		GET_FS(lastError);
		int res = fs->tell(handle);
		check(res);
		return res;
	}

	/**
	 * @brief Truncate (reduce) the size of an open file
	 * @param newSize
     * @retval bool true on success
	 */
	bool truncate(size_t new_size)
	{
		GET_FS(lastError);
		int res = fs->ftruncate(handle, new_size);
		return check(res);
	}

	/**
	 * @brief Truncate an open file at the current cursor position
     * @retval bool true on success
	 */
	bool truncate()
	{
		GET_FS(lastError);
		int res = fs->ftruncate(handle);
		return check(res);
	}

	/**
	 * @brief flush any buffered data to physical media
     * @retval bool true on success
     */
	bool flush()
	{
		GET_FS(false);
		return check(fs->flush(handle));
	}

	/**
	 * @brief Set access control information for file
     * @param acl
     * @retval bool true on success
     */
	bool setacl(const ACL& acl)
	{
		GET_FS(false);
		return check(fs->setacl(handle, acl));
	}

	/**
	 * @brief Set modification time for file
     * @retval bool true on success
     * @note any subsequent writes to file will reset this to current time
     */
	bool settime(time_t mtime)
	{
		GET_FS(false);
		return check(fs->settime(handle, mtime));
	}

	/**
	 * @brief Set file compression information
     * @param compression
     * @retval bool true on success
     */
	bool setcompression(const Compression& compression)
	{
		GET_FS(false);
		return check(fs->setcompression(handle, compression));
	}

	/**
	 * @brief remove (delete) an open file (and close it)
     * @retval bool true on success
     */
	bool remove()
	{
		GET_FS(false);
		int res = fs->fremove(handle);
		if(!check(res)) {
			return false;
		}
		fs->close(handle);
		handle = -1;
		return true;
	}

	/**
	 * @brief  Get size of file
	 * @retval uint32_t Size of file in bytes, 0 on error
	 */
	uint32_t getSize()
	{
		GET_FS(lastError);
		return fs->getSize(handle);
	}

	using ReadContentCallback = FileSystem::ReadContentCallback;

	/**
	 * @brief Read from current file position and invoke callback for each block read
	 * @param size Maximum number of bytes to read
	 * @param callback
	 * @retval int Number of bytes processed, or error code
	 */
	int readContent(size_t size, ReadContentCallback callback)
	{
		GET_FS(lastError);
		return fs->readContent(handle, size, callback);
	}

	/**
	 * @brief Read from current file position to end of file and invoke callback for each block read
	 * @param callback
	 * @retval int Number of bytes processed, or error code
	 */
	int readContent(ReadContentCallback callback)
	{
		GET_FS(lastError);
		return fs->readContent(handle, callback);
	}

	/**
	 * @brief  Read content of a file
	 * @param  fileName Name of file to read from
	 * @retval String String variable in to which to read the file content
	 * @note   After calling this function the content of the file is placed in to a string.
	 * The result will be an invalid String (equates to `false`) if the file could not be read.
	 * If the file exists, but is empty, the result will be an empty string "".
	 */
	String getContent()
	{
		String s;
		auto len = getSize();
		if(!s.setLength(len)) {
			return nullptr;
		}

		if(read(s.begin(), len) != int(len)) {
			return nullptr;
		}

		return s;
	}

private:
	FileHandle handle{-1};
};

}; // namespace IFS
