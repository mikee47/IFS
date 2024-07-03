/****
 * Control.h
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
	 * @brief Set volume label
	*/
	FCNTL_SET_VOLUME_LABEL = 2,
	/**
	 * @brief Start of user-defined codes
	 *
	 * Codes before this are reserved for system use
	 */
	FCNTL_USER_BASE = 0x8000,
};

} // namespace IFS
