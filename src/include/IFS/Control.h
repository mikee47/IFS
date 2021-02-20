#pragma once

#include <cstdint>

namespace IFS
{
/**
 * @brief See `IFS::IFileSystem::fcontrol`
 *
 * These are weakly typed as values may be defined elsewhere.
 */
enum ControlCode : uint16_t {
	/**
	 * @brief Get stored MD5 hash for file
	 * 
	 * FWFS calculates this when the filesystem image is built and can be used
	 * by applications to verify file integrity.
	 *
	 * On success, returns size of the hash itself (16)
	 * If the file is zero-length then Error::NotFound will be returned as the hash
	 * is not stored for empty files.
	 */
	FCNTL_GET_MD5_HASH = 1,
	/**
	 * @brief Start of user-defined codes
	 *
	 * Codes before this are reserved for system use
	 */
	FCNTL_USER_BASE = 0x8000,
};

} // namespace IFS
