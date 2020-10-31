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

#include "File/Stat.h"
#include "File/OpenFlags.h"
#include "File/SeekOrigin.h"
#include "Media.h"
#include "Error.h"

/**
 * @brief Four-character tag to identify type of filing system
 * @note As new filing systems are incorporated into IFS, update this enumeration
 */
#define FILESYSTEM_TYPE_MAP(XX)                                                                                        \
	XX(Unknown, NULL, "Unknown")                                                                                       \
	XX(FWFS, FWFS, "Firmware File System")                                                                             \
	XX(SPIFFS, SPIF, "SPI Flash File System (SPIFFS)")                                                                 \
	XX(Hybrid, HYFS, "Hybrid File System")

/**
 * @brief Attribute flags for filing system
 */
#define FILE_SYSTEM_ATTR_MAP(XX)                                                                                       \
	XX(Mounted, "Filing system is mounted and in use")                                                                 \
	XX(ReadOnly, "Writing not permitted to this volume")                                                               \
	XX(Virtual, "Virtual filesystem, doesn't host files directly")                                                     \
	XX(Check, "Volume check recommended")

namespace IFS
{
class IFileSystem;

/*
 * Opaque structure for directory reading
 */
using DirHandle = struct FileDir*;

/**
 * @brief Get current timestamp in UTC
 * @retval time_t
 * @note Filing systems must store timestamps in UTC
 * Use this function; makes porting easier.
 */
time_t fsGetTimeUTC();

#if DEBUG_BUILD
#define debug_ifserr(_err, _func, ...)                                                                                 \
	do {                                                                                                               \
		int err = _err;                                                                                                \
		debug_e(_func ": %s (%d)", ##__VA_ARGS__, getErrorString(err).c_str(), err);                                   \
	} while(0)
#else
#define debug_ifserr(_err, _func, ...)                                                                                 \
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
		Type type{};				 ///< The filing system type identifier
		const Media* media{nullptr}; ///< WARNING: can be null for virtual file systems
		Attributes attr{};			 ///< Attribute flags
		uint32_t volumeID{0};		 ///< Unique identifier for volume
		NameBuffer name;			 ///< Buffer for name
		uint32_t volumeSize{0};		 ///< Size of volume, in bytes
		uint32_t freeSpace{0};		 ///< Available space, in bytes

		Info()
		{
		}

		Info(char* namebuf, unsigned buflen) : name(namebuf, buflen)
		{
		}

		Info& operator=(const Info& rhs)
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
			*this = Info{};
		}
	};

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
	virtual int getinfo(Info& info) = 0;

	/**
	 * @brief get the text for a returned error code
     * @retval String
     */
	virtual String getErrorString(int err)
	{
		return Error::toString(err);
	}

	/**
	 * @name open a directory for reading
     * @param path path to directory. nullptr is interpreted as root directory
     * @param dir returns a pointer to the directory object
     * @retval int error code
	 * @{
     */
	virtual int opendir(const char* path, DirHandle& dir) = 0;

	int opendir(const String& path, DirHandle& dir)
	{
		return opendir(path.c_str(), dir);
	}

	/** @} */

	/**
	 * @brief open a directory for reading
     * @param stat identifies directory to open. nullptr is interpreted as root directory
     * @param dir returns a pointer to the directory object
     * @retval int error code
     */
	virtual int fopendir(const File::Stat* stat, DirHandle& dir)
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
	virtual int readdir(DirHandle dir, File::Stat& stat) = 0;

	/**
	 * @brief close a directory object
     * @param dir directory to close
     * @retval int error code
     */
	virtual int closedir(DirHandle dir) = 0;

	/**
	 * @name get file information
     * @param path name or path of file
     * @param s structure to return information in, may be null to do a simple file existence check
     * @retval int error code
     * @{
     */
	virtual int stat(const char* path, File::Stat* stat) = 0;

	int stat(const String& path, File::Stat* s)
	{
		return stat(path.c_str(), s);
	}

	/** @} */

	/**
	 * @name get file information
     * @param file handle to open file
     * @param stat structure to return information in, may be null
     * @retval int error code
     * @{
     */
	virtual int fstat(File::Handle file, File::Stat* stat) = 0;

	int fstat(File::Handle file, File::Stat& stat)
	{
		return fstat(file, &stat);
	}

	/** @} */

	/**
	 * @name open a file by name/path
     * @param path full path to file
     * @param flags opens for opening file
     * @retval File::Handle file handle or error code
     * @{
     */
	virtual File::Handle open(const char* path, File::OpenFlags flags) = 0;

	File::Handle open(const String& path, File::OpenFlags flags)
	{
		return open(path.c_str(), flags);
	}

	/** @} */

	/**
	 * @brief open a file from it's stat structure
     * @param stat obtained from readdir()
     * @param flags opens for opening file
     * @retval File::Handle file handle or error code
     */
	virtual File::Handle fopen(const File::Stat& stat, File::OpenFlags flags) = 0;

	/**
	 * @brief close an open file
     * @param file handle to open file
     * @retval int error code
     */
	virtual int close(File::Handle file) = 0;

	/**
	 * @brief read content from a file and advance cursor
     * @param file handle to open file
     * @param data buffer to write into
     * @param size size of file buffer, maximum number of bytes to read
     * @retval int number of bytes read or error code
     */
	virtual int read(File::Handle file, void* data, size_t size) = 0;

	/**
	 * @brief write content to a file at current position and advance cursor
     * @param file handle to open file
     * @param data buffer to read from
     * @param size number of bytes to write
     * @retval int number of bytes written or error code
     */
	virtual int write(File::Handle file, const void* data, size_t size) = 0;

	/**
	 * @brief change file read/write position
     * @param file handle to open file
     * @param offset position relative to origin
     * @param origin where to seek from (start/end or current position)
     * @retval int current position or error code
     */
	virtual int lseek(File::Handle file, int offset, File::SeekOrigin origin) = 0;

	/**
	 * @brief determine if current file position is at end of file
     * @param file handle to open file
     * @retval int 0 - not EOF, > 0 - at EOF, < 0 - error
     */
	virtual int eof(File::Handle file) = 0;

	/**
	 * @brief get current file position
     * @param file handle to open file
     * @retval int32_t current position relative to start of file, or error code
     */
	virtual int32_t tell(File::Handle file) = 0;

	/**
	 * @brief Truncate the file at the current cursor position
     * @param file handle to open file
     * @retval int new file size, or error code
     * @note Changes the file size
     */
	virtual int truncate(File::Handle file, size_t new_size) = 0;

	/**
	 * @brief flush any buffered data to physical media
     * @param file handle to open file
     * @retval int error code
     */
	virtual int flush(File::Handle file) = 0;

	/**
	 * @brief Set access control information for file
     * @param file handle to open file
     * @param acl
     * @retval int error code
     */
	virtual int setacl(File::Handle file, const File::ACL& acl) = 0;

	/**
	 * @brief Set file attributes
     * @param file handle to open file, must have write access
     * @param attr
     * @retval int error code
     */
	virtual int setattr(File::Handle file, File::Attributes attr) = 0;

	/**
	 * @brief Set access control information for file
     * @param file handle to open file, must have write access
     * @retval int error code
     * @note any writes to file will reset this to current time
     */
	virtual int settime(File::Handle file, time_t mtime) = 0;

	/**
	 * @name rename a file
     * @param oldpath
     * @param newpath
     * @retval int error code
     * @{
     */
	virtual int rename(const char* oldpath, const char* newpath) = 0;

	int rename(const String& oldpath, const String& newpath)
	{
		return rename(oldpath.c_str(), newpath.c_str());
	}

	/** @} */

	/**
	 * @name remove (delete) a file by path
     * @param path
     * @retval int error code
     * @{
     */
	virtual int remove(const char* path) = 0;

	int remove(const String& path)
	{
		return remove(path.c_str());
	}

	/** @} */

	/**
	 * @brief remove (delete) a file by handle
     * @param file handle to open file
     * @retval int error code
     */
	virtual int fremove(File::Handle file) = 0;

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

	/**
	 * @brief Determine if the given file handle is valid
	 * @param file handle to check
	 * @retval int error code
	 * @note error code typically Error::InvalidHandle if handle is outside valid range,
	 * or Error::FileNotOpen if handle range is valid but handle not in use
	 */
	virtual int isfile(File::Handle file) = 0;
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
