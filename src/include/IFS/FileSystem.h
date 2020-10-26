/****
 * Installable File System
 *
 * Created August 2018 by mikee471
 *
 ****/

/**	@defgroup filesystem File system
 *	@brief	Installable file system
 *  @{
 */

#pragma once

#include <time.h>
#include <string.h>
#include "FileOpenFlags.h"
#include "FileSystemType.h"
#include "FileSystemAttributes.h"
#include "FileAttributes.h"
#include "Types.h"
#include "Access.h"
#include "Compression.h"
#include "Media.h"
#include "Error.h"
#include "NameBuffer.h"

namespace IFS
{
/*
 * File handle
 *
 * References an open file
 */
using file_t = int16_t;

/*
 * File identifier
 *
 * Contained within FileStat, uniquely identifies any file on the file system.
 */
using fileid_t = uint32_t;

/** @brief File seek origin flags
 *  @note these values are fixed in stone so will never change. They only need to
 *  be remapped if a filing system uses different values.
 */
enum class SeekOrigin {
	Start = 0,   ///< Start of file
	Current = 1, ///< Current position in file
	End = 2		 ///< End of file
};

class IFileSystem;

/*
 * File Status structure
 */
struct FileStat {
	IFileSystem* fs{nullptr}; ///< The filing system owning this file
	NameBuffer name;		  ///< Name of file
	uint32_t size{0};		  ///< Size of file in bytes
	fileid_t id{0};			  ///< Internal file identifier
	CompressionType compression{};
	FileAttributes attr{0};
	FileACL acl = {UserRole::None, UserRole::None}; ///< Access Control
	time_t mtime{0};								///< File modification time

	FileStat()
	{
	}

	FileStat(char* namebuf, uint16_t bufsize) : name(namebuf, bufsize)
	{
	}

	/** @brief assign content from another FileStat structure
	 *  @note All fields are copied as for a normal assignment, except for 'name', where
	 *  rhs.name contents are copied into our name buffer.
	 */
	FileStat& operator=(const FileStat& rhs)
	{
		fs = rhs.fs;
		name.copy(rhs.name);
		size = rhs.size;
		id = rhs.id;
		compression = rhs.compression;
		attr = rhs.attr;
		acl = rhs.acl;
		mtime = rhs.mtime;
		return *this;
	}

	void clear()
	{
		*this = FileStat{};
	}
};

/**
 * @brief version of FileStat with integrated name buffer
 * @note provide for convenience
 */
struct FileNameStat : public FileStat {
public:
	FileNameStat() : FileStat(buffer, sizeof(buffer))
	{
	}

private:
	char buffer[256];
};

/**
 * @brief Opaque structure for directory reading
 */
using filedir_t = struct FileDir*;

/**
 * @brief Get current timestamp in UTC
 * @retval time_t
 * @note Filing systems must store timestamps in UTC
 * Use this function; makes porting easier.
 */
time_t fsGetTimeUTC();

/**
 * @brief Basic information about filing system
 */
struct FileSystemInfo {
	FileSystemType type{};		 ///< The filing system type identifier
	const Media* media{nullptr}; ///< WARNING: can be null for virtual file systems
	FileSystemAttributes attr{}; ///< Attribute flags
	uint32_t volumeID{0};		 ///< Unique identifier for volume
	NameBuffer name;			 ///< Buffer for name
	uint32_t volumeSize{0};		 ///< Size of volume, in bytes
	uint32_t freeSpace{0};		 ///< Available space, in bytes

	FileSystemInfo()
	{
	}

	FileSystemInfo(char* namebuf, unsigned buflen) : name(namebuf, buflen)
	{
	}

	FileSystemInfo& operator=(const FileSystemInfo& rhs)
	{
		type = rhs.type;
		media = rhs.media;
		attr = rhs.attr;
		volumeID = rhs.volumeID;
		name.copy(rhs.name);
		volumeSize = rhs.volumeSize;
		freeSpace = rhs.freeSpace;
		return *this;
	}

	void clear()
	{
		*this = FileSystemInfo{};
	}
};

#if DEBUG_BUILD
#define debug_ifserr(_err, _func, ...)                                                                                 \
	do {                                                                                                               \
		int err = _err;                                                                                                \
		char buf[64];                                                                                                  \
		geterrortext(err, buf, sizeof(buf));                                                                           \
		debug_e(_func ": %s (%d)", ##__VA_ARGS__, buf, err);                                                           \
	} while(0)
#else
#define debug_ifserr(_err, _func, ...)                                                                                 \
	do {                                                                                                               \
	} while(0)
#endif

/** @brief Installable File System base class
 *  @note The 'I' implies Installable but could be for Interface :-)
 *
 * Construction and initialisation of a filing system is implementation-dependent so there
 * are no methods here for that.
 *
 * Methods are defined as virtual abstract unless we actually have a default base implementation.
 * Whilst some methods could just return FSERR_NotImplemented by default, keeping them abstract forces
 * all file system implementations to consider them so provides an extra check for completeness.
 *
 */
class IFileSystem
{
public:
	/**
	 * @brief Filing system implementations should dismount and cleanup here
	 */
	virtual ~IFileSystem()
	{
	}

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
	virtual int getinfo(FileSystemInfo& info) = 0;

	/**
	 * @brief get the text for a returned error code
     * @param buffer to place text
     * @param size bytes available in buffer
     * @retval int length of text, excluding nul terminator, or negative error code
     * @note nul terminator must always be written to buffer, even on error, unless
     * buffer is null and/or size is 0. File system implementations should call
     * this method if unable to resolve the error code.
     */
	virtual int geterrortext(int err, char* buffer, size_t size)
	{
		return fsGetErrorText(err, buffer, size);
	}

	/**
	 * @brief open a directory for reading
     * @param path path to directory. nullptr is interpreted as root directory
     * @param dir returns a pointer to the directory object
     * @retval int error code
     */
	virtual int opendir(const char* path, filedir_t* dir) = 0;

	/**
	 * @brief open a directory for reading
     * @param stat identifies directory to open. nullptr is interpreted as root directory
     * @param dir returns a pointer to the directory object
     * @retval int error code
     */
	virtual int fopendir(const FileStat* stat, filedir_t* dir)
	{
		return opendir(stat == nullptr ? nullptr : stat->name.buffer, dir);
	}

	/**
	 * @brief read a directory entry
     * @param dir
     * @param stat
     * @retval int error code
     * @note File system allocates entries structure as it usually needs
     * to track other information. It releases memory when closedir() is called.
     */
	virtual int readdir(filedir_t dir, FileStat* stat) = 0;

	/**
	 * @brief close a directory object
     * @param dir directory to close
     * @retval int error code
     */
	virtual int closedir(filedir_t dir) = 0;

	/**
	 * @brief get file information
     * @param path name or path of file
     * @param stat structure to return information in, may be null to do a simple file existence check
     * @retval int error code
     */
	virtual int stat(const char* path, FileStat* stat) = 0;

	/**
	 * @brief get file information
     * @param file handle to open file
     * @param stat structure to return information in, may be null
     * @retval int error code
     */
	virtual int fstat(file_t file, FileStat* stat) = 0;

	/**
	 * @brief open a file by name/path
     * @param path full path to file
     * @param flags opens for opening file
     * @retval file_t file handle or error code
     */
	virtual file_t open(const char* path, FileOpenFlags flags) = 0;

	/**
	 * @brief open a file from it's stat structure
     * @param stat obtained from readdir()
     * @param flags opens for opening file
     * @retval file_t file handle or error code
     */
	virtual file_t fopen(const FileStat& stat, FileOpenFlags flags) = 0;

	/**
	 * @brief close an open file
     * @param file handle to open file
     * @retval int error code
     */
	virtual int close(file_t file) = 0;

	/**
	 * @brief read content from a file and advance cursor
     * @param file handle to open file
     * @param data buffer to write into
     * @param size size of file buffer, maximum number of bytes to read
     * @retval int number of bytes read or error code
     */
	virtual int read(file_t file, void* data, size_t size) = 0;

	/**
	 * @brief write content to a file at current position and advance cursor
     * @param file handle to open file
     * @param data buffer to read from
     * @param size number of bytes to write
     * @retval int number of bytes written or error code
     */
	virtual int write(file_t file, const void* data, size_t size) = 0;

	/**
	 * @brief change file read/write position
     * @param file handle to open file
     * @param offset position relative to origin
     * @param origin where to seek from (start/end or current position)
     * @retval int current position or error code
     */
	virtual int lseek(file_t file, int offset, SeekOrigin origin) = 0;

	/**
	 * @brief determine if current file position is at end of file
     * @param file handle to open file
     * @retval int 0 - not EOF, > 0 - at EOF, < 0 - error
     */
	virtual int eof(file_t file) = 0;

	/**
	 * @brief get current file position
     * @param file handle to open file
     * @retval int32_t current position relative to start of file, or error code
     */
	virtual int32_t tell(file_t file) = 0;

	/**
	 * @brief Truncate the file at the current cursor position
     * @param file handle to open file
     * @retval int new file size, or error code
     * @note Changes the file size
     */
	virtual int truncate(file_t file, size_t new_size) = 0;

	/**
	 * @brief flush any buffered data to physical media
     * @param file handle to open file
     * @retval int error code
     */
	virtual int flush(file_t file) = 0;

	/**
	 * @brief Set access control information for file
     * @param file handle to open file
     * @retval int error code
     */
	virtual int setacl(file_t file, FileACL* acl) = 0;

	/**
	 * @brief Set file attributes
     * @param file handle to open file, must have write access
     * @retval int error code
     */
	virtual int setattr(file_t file, FileAttributes attr) = 0;

	/**
	 * @brief Set access control information for file
     * @param file handle to open file, must have write access
     * @retval int error code
     * @note any writes to file will reset this to current time
     */
	virtual int settime(file_t file, time_t mtime) = 0;

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
	virtual int fremove(file_t file) = 0;

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
		return FSERR_NotImplemented;
	}

	/**
	 * @brief Determine if the given file handle is valid
	 * @param file handle to check
	 * @retval int error code
	 * @note error code typically FSERR_InvalidHandle if handle is outside valid range,
	 * or FSERR_FileNotOpen if handle range is valid but handle not in use
	 */
	virtual int isfile(file_t file) = 0;
};

} // namespace IFS

/** @} */
