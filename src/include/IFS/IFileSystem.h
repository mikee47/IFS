/****
 * IFileSystem.h
 * Abstract filesystem interface definitions
 *
 * Created: August 2018
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

#include "Stat.h"
#include "OpenFlags.h"
#include <Storage/Partition.h>
#include "Error.h"
#include "Control.h"
#include "Profiler.h"
#include "Attribute.h"
#include "Extent.h"
#include <Data/Stream/SeekOrigin.h>

/**
 * @brief Four-character tag to identify type of filing system
 * @note As new filing systems are incorporated into IFS, update this enumeration
 */
#define FILESYSTEM_TYPE_MAP(XX)                                                                                        \
	XX(Unknown, NULL, "Unknown")                                                                                       \
	XX(FWFS, FWFS, "Firmware File System")                                                                             \
	XX(SPIFFS, SPIF, "SPI Flash File System (SPIFFS)")                                                                 \
	XX(LittleFS, LFS, "Little FS")                                                                                     \
	XX(Hybrid, HYFS, "Hybrid File System")                                                                             \
	XX(Host, HOST, "Host File System")                                                                                 \
	XX(Fat, FAT, "FAT File System")                                                                                    \
	XX(Fat32, FAT32, "FAT32 File System")                                                                              \
	XX(ExFat, exFAT, "EXFAT File System")

/**
 * @brief Attribute flags for filing system
 */
#define FILE_SYSTEM_ATTR_MAP(XX)                                                                                       \
	XX(Mounted, "Filing system is mounted and in use")                                                                 \
	XX(ReadOnly, "Writing not permitted to this volume")                                                               \
	XX(Virtual, "Virtual filesystem, doesn't host files directly")                                                     \
	XX(Check, "Volume check recommended")                                                                              \
	XX(NoMeta, "Metadata unsupported")

namespace IFS
{
class IFileSystem;

/*
 * Opaque structure for directory reading
 */
using DirHandle = struct ImplFileDir*;

#if DEBUG_BUILD
#define debug_ifserr(err, func, ...)                                                                                   \
	do {                                                                                                               \
		[[maybe_unused]] int errorCode = err;                                                                          \
		debug_e(func ": %s (%d)", ##__VA_ARGS__, getErrorString(errorCode).c_str(), err);                              \
	} while(0)
#else
#define debug_ifserr(err, func, ...)                                                                                   \
	do {                                                                                                               \
	} while(0)
#endif

/**
 * @brief Installable File System base class
 * @note The 'I' implies Installable but could be for Interface :-)
 *
 * Construction and initialisation of a filing system is implementation-dependent so there
 * are no methods here for that.
 *
 * Methods are defined as virtual abstract unless we actually have a default base implementation.
 * Whilst some methods could just return Error::NotImplemented by default, keeping them abstract forces
 * all file system implementations to consider them so provides an extra check for completeness.
 *
 */
class IFileSystem
{
public:
	enum class Type {
#define XX(_name, _tag, _desc) _name,
		FILESYSTEM_TYPE_MAP(XX)
#undef XX
			MAX
	};

	enum class Attribute {
#define XX(_tag, _comment) _tag,
		FILE_SYSTEM_ATTR_MAP(XX)
#undef XX
			MAX
	};

	// The set of attributes
	using Attributes = BitSet<uint8_t, Attribute, size_t(Attribute::MAX)>;

	/**
	 * @brief Basic information about filing system
	 */
	struct Info {
		Type type{};			   ///< The filing system type identifier
		Attributes attr{};		   ///< Attribute flags
		size_t maxNameLength{255}; ///< Maximum length of a single file name
		size_t maxPathLength{255}; ///< Maximum length of a full file path
		Storage::Partition partition;
		uint32_t volumeID{0};		 ///< Unique identifier for volume
		NameBuffer name;			 ///< Buffer for name
		volume_size_t volumeSize{0}; ///< Size of volume, in bytes
		volume_size_t freeSpace{0};  ///< Available space, in bytes
		TimeStamp creationTime{};

		Info() = default;

		Info(char* namebuf, unsigned buflen) : name(namebuf, buflen)
		{
		}

		volume_size_t used() const
		{
			return volumeSize - freeSpace;
		}

		Info& operator=(const Info& rhs)
		{
			type = rhs.type;
			partition = rhs.partition;
			attr = rhs.attr;
			volumeID = rhs.volumeID;
			name.copy(rhs.name);
			volumeSize = rhs.volumeSize;
			freeSpace = rhs.freeSpace;
			return *this;
		}

		void clear()
		{
			*this = Info{};
		}

		size_t printTo(Print& p) const;
	};

	/**
	 * @brief Filing system information with buffer for name
	 */
	struct NameInfo : public Info {
	public:
		NameInfo() : Info(buffer, sizeof(buffer))
		{
		}

	private:
		char buffer[256];
	};

	/**
	 * @brief Filing system implementations should dismount and cleanup here
	 */
	virtual ~IFileSystem() = default;

	/**
	 * @brief Mount file system, performing any required initialisation
	 * @retval error code
	 */
	virtual int mount() = 0;

	/**
	 * @brief get filing system information
     * @param info structure to read information into
     * @retval int error code
     */
	virtual int getinfo(Info& info) = 0;

	/**
	 * @brief Set profiler instance to enable debugging and performance assessment
	 * @param profiler
	 * @retval int error code - profiling may not be supported on all filesystems
     */
	virtual int setProfiler(IProfiler*)
	{
		return Error::NotImplemented;
	}

	/**
	 * @brief get the text for a returned error code
     * @retval String
     */
	virtual String getErrorString(int err)
	{
		return Error::toString(err);
	}

	/**
	 * @brief Set volume for mountpoint
	 * @param index Volume index
	 * @param fileSystem The filesystem to root at this mountpoint
	 * @retval int error code
	 */
	virtual int setVolume([[maybe_unused]] uint8_t index, [[maybe_unused]] IFileSystem* fileSystem)
	{
		return Error::NotSupported;
	}

	/**
	 * @brief open a directory for reading
     * @param path path to directory. nullptr is interpreted as root directory
     * @param dir returns a pointer to the directory object
     * @retval int error code
     */
	virtual int opendir(const char* path, DirHandle& dir) = 0;

	/**
	 * @brief read a directory entry
     * @param dir
     * @param stat
     * @retval int error code
     * @note File system allocates entries structure as it usually needs
     * to track other information. It releases memory when closedir() is called.
     */
	virtual int readdir(DirHandle dir, Stat& stat) = 0;

	/**
	 * @brief Reset directory read position to start
     * @param dir
     * @retval int error code
     */
	virtual int rewinddir(DirHandle dir) = 0;

	/**
	 * @brief close a directory object
     * @param dir directory to close
     * @retval int error code
     */
	virtual int closedir(DirHandle dir) = 0;

	/**
	 * @brief Create a directory
	 * @param path Path to directory
	 * @retval int error code
	 *
	 * Only the final directory in the path is guaranteed to be created.
	 * Usually, this call will fail if intermediate directories are not present.
	 * Use `IFS::FileSystem::makedirs()` for this purpose.
	 */
	virtual int mkdir(const char* path) = 0;

	/**
	 * @brief get file information
     * @param path name or path of file/directory/mountpoint
     * @param s structure to return information in, may be null to do a simple file existence check
     * @retval int error code
	 *
	 * Returned stat will indicate whether the path is a mountpoint or directory.
	 * For a mount point, stats for the root directory of the mounted filesystem
	 * must be obtained by opening a handle then using `fstat`:
	 *
	 * ```
	 * int handle = fs.open("path-to-mountpoint");
	 * Stat stat;
	 * fs.fstat(handle, &stat);
	 * fs.close(handle);
	 * ```
     */
	virtual int stat(const char* path, Stat* stat) = 0;

	/**
	 * @brief get file information
     * @param file handle to open file
     * @param stat structure to return information in, may be null
     * @retval int error code
     */
	virtual int fstat(FileHandle file, Stat* stat) = 0;

	/**
	 * @brief Low-level and non-standard file control operations
	 * @param file
	 * @param code FCNTL_XXX code
	 * @param buffer Input/Output buffer
	 * @param bufSize Size of buffer
	 * @retval int error code or, on success, data size
	 * 
	 * To simplify usage the same buffer is used for both input and output.
	 * Only the size of the buffer is provided. If a specific FCNTL code requires more
	 * information then it will be contained within the provided data.
	 */
	virtual int fcontrol([[maybe_unused]] FileHandle file, [[maybe_unused]] ControlCode code,
						 [[maybe_unused]] void* buffer, [[maybe_unused]] size_t bufSize)
	{
		return Error::NotSupported;
	}

	/**
	 * @brief open a file (or directory) by path
     * @param path full path to file
     * @param flags Desired access and other options
     * @retval FileHandle file handle or error code
	 *
	 * This function may also be used to obtain a directory handle to perform various operations
	 * such as enumerating attributes.
	 * Calls to read or write on such handles will typically fail.
     */
	virtual FileHandle open(const char* path, OpenFlags flags) = 0;

	/**
	 * @brief close an open file
     * @param file handle to open file
     * @retval int error code
     */
	virtual int close(FileHandle file) = 0;

	/**
	 * @brief read content from a file and advance cursor
     * @param file handle to open file
     * @param data buffer to write into
     * @param size size of file buffer, maximum number of bytes to read
     * @retval int number of bytes read or error code
     */
	virtual int read(FileHandle file, void* data, size_t size) = 0;

	/**
	 * @brief write content to a file at current position and advance cursor
     * @param file handle to open file
     * @param data buffer to read from
     * @param size number of bytes to write
     * @retval int number of bytes written or error code
     */
	virtual int write(FileHandle file, const void* data, size_t size) = 0;

	/**
	 * @brief change file read/write position
     * @param file handle to open file
     * @param offset position relative to origin
     * @param origin where to seek from (start/end or current position)
     * @retval file_offset_t current position or error code
     */
	virtual file_offset_t lseek(FileHandle file, file_offset_t offset, SeekOrigin origin) = 0;

	/**
	 * @brief determine if current file position is at end of file
     * @param file handle to open file
     * @retval int 0 - not EOF, > 0 - at EOF, < 0 - error
     */
	virtual int eof(FileHandle file) = 0;

	/**
	 * @brief get current file position
     * @param file handle to open file
     * @retval file_offset_t current position relative to start of file, or error code
     */
	virtual file_offset_t tell(FileHandle file) = 0;

	/**
	 * @brief Truncate (reduce) the size of an open file
	 * @param file Open file handle, must have Write access
	 * @param newSize
	 * @retval int Error code
	 * @note In POSIX `ftruncate()` can also make the file bigger, however SPIFFS can only
	 * reduce the file size and will return an error if newSize > fileSize
	 */
	virtual int ftruncate(FileHandle file, file_size_t new_size) = 0;

	/**
	 * @brief flush any buffered data to physical media
     * @param file handle to open file
     * @retval int error code
     */
	virtual int flush(FileHandle file) = 0;

	/**
	 * @brief Set an extended attribute on an open file
     * @param file handle to open file
	 * @param tag The attribute to write
	 * @param data Content of the attribute. Pass nullptr to remove the attribute (if possible).
	 * @param size Size of the attribute in bytes
     * @retval int error code
	 * @note Attributes may not be written to disk until flush() or close() are called
     */
	virtual int fsetxattr(FileHandle file, AttributeTag tag, const void* data, size_t size) = 0;

	/**
	 * @brief Get an extended attribute from an open file
     * @param file handle to open file
	 * @param tag The attribute to read
	 * @param buffer Buffer to receive attribute content
	 * @param size Size of the buffer
     * @retval int error code, on success returns size of attribute (which may be larger than size)
     */
	virtual int fgetxattr(FileHandle file, AttributeTag tag, void* buffer, size_t size) = 0;

	/**
	 * @brief Enumerate attributes
     * @param file handle to open file
	 * @param callback Callback function to invoke for each attribute found
	 * @param buffer Buffer to use for reading attribute data. Use nullptr if only tags are required
	 * @param bufsize Size of buffer
     * @retval int error code, on success returns number of attributes read
     */
	virtual int fenumxattr(FileHandle file, AttributeEnumCallback callback, void* buffer, size_t bufsize) = 0;

	/**
	 * @brief Set an extended attribute for a file given its path
     * @param path Full path to file (or directory)
	 * @param tag The attribute to write
	 * @param data Content of the attribute. Pass nullptr to remove the attribute (if possible).
	 * @param size Size of the attribute in bytes
     * @retval int error code
     */
	virtual int setxattr(const char* path, AttributeTag tag, const void* data, size_t size) = 0;

	/**
	 * @brief Get an attribute from a file given its path
     * @param file Full path to file (or directory)
	 * @param tag The attribute to read
	 * @param buffer Buffer to receive attribute content
	 * @param size Size of the buffer
     * @retval int error code, on success returns size of attribute (which may be larger than size)
     */
	virtual int getxattr(const char* path, AttributeTag tag, void* buffer, size_t size) = 0;

	/**
	 * @brief Get extents for a file
     * @param file Handle to open file
	 * @param part Partition where the file lives (OUT, OPTIONAL)
	 * @param list Buffer for extents (OPTIONAL)
	 * @param extcount Maximum number of extents to return in `list`
     * @retval int Total number of extents for file (may be larger than 'extcount'), or error code
     */
	virtual int fgetextents([[maybe_unused]] FileHandle file, [[maybe_unused]] Storage::Partition* part,
							[[maybe_unused]] Extent* list, [[maybe_unused]] uint16_t extcount)
	{
		return Error::NotImplemented;
	}

	/**
	 * @brief rename a file
     * @param oldpath
     * @param newpath
     * @retval int error code
     */
	virtual int rename(const char* oldpath, const char* newpath) = 0;

	/**
	 * @brief remove (delete) a file by path
     * @param path
     * @retval int error code
     */
	virtual int remove(const char* path) = 0;

	/**
	 * @brief remove (delete) a file by handle
     * @param file handle to open file
     * @retval int error code
     */
	virtual int fremove(FileHandle file) = 0;

	/**
	 * @brief format the filing system
     * @retval int error code
     * @note this does a default format, returning file system to a fresh state
     * The filing system implementation may define more specialised methods
     * which can be called directly.
     */
	virtual int format() = 0;

	/**
	 * @brief Perform a file system consistency check
     * @retval int error code
     * @note if possible, issues should be resolved. Returns 0 if file system
     * checked out OK. Otherwise there were issues: < 0 for unrecoverable errors,
     * > 0 for recoverable errors.
     */
	virtual int check()
	{
		return Error::NotImplemented;
	}
};

} // namespace IFS

/**
 * @brief Get String for filesystem type
 */
String toString(IFS::IFileSystem::Type type);

/**
 * @brief Get String for a filesystem attribute
 */
String toString(IFS::IFileSystem::Attribute attr);

/** @} */
