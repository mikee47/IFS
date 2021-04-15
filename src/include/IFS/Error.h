/**
 * Error.h
 * IFS Error codes and related stuff
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

/**
 * @defgroup filesystem File system
 * @brief	Installable file system
 * @{
 */

#pragma once

#include "Types.h"

namespace IFS
{
using ErrorCode = int;

namespace Error
{
/**
 * @brief IFS return codes
 *
 * An IFS implementation must return negative values for errors. Wrappers may
 * use these IFS codes instead of their own.
 * If returning an internal error code it may need to be translated.
 * Methods returning ONLY an error code (i.e. not FileHandle) may return positive 'error'
 * codes for information purposes. See IFileSystem::check() as an example.
 * Return value usage is consistent with SPIFFS.
 *
 * These codes are allocated using an enum since they are purely for internal purposes.
 */
#define IFS_ERROR_MAP(XX)                                                                                              \
	XX(Success, "Success")                                                                                             \
	XX(NoFileSystem, "File system has not been set")                                                                   \
	XX(NoPartition, "Partition not found / undefined")                                                                 \
	XX(BadPartition, "Partition is not valid for this filesystem")                                                     \
	XX(BadVolumeIndex, "Volume index is invalid")                                                                      \
	XX(NotMounted, "File system is not mounted")                                                                       \
	XX(BadObject, "Malformed filesystem object")                                                                       \
	XX(BadFileSystem, "File system corruption detected")                                                               \
	XX(BadParam, "Invalid parameter(s)")                                                                               \
	XX(NotImplemented, "File system or method not yet implemented")                                                    \
	XX(NotSupported, "Parameter value not supported")                                                                  \
	XX(NoMem, "Memory allocation failed")                                                                              \
	XX(NameTooLong, "File name or path is too long for buffer")                                                        \
	XX(BufferTooSmall, "Data is too long for buffer")                                                                  \
	XX(NotFound, "Object not found")                                                                                   \
	XX(ReadOnly, "Media is read-only")                                                                                 \
	XX(ReadFailure, "Media read failed")                                                                               \
	XX(WriteFailure, "Media write failed")                                                                             \
	XX(EraseFailure, "Media erase failed")                                                                             \
	XX(InvalidHandle, "File handle is outside valid range")                                                            \
	XX(InvalidObjectRef, "Filesystem object reference invalid")                                                        \
	XX(InvalidObject, "Filesystem object invalid")                                                                     \
	XX(EndOfObjects, "Last object in filing system has been read")                                                     \
	XX(FileNotOpen, "File handle is valid but the file is not open")                                                   \
	XX(SeekBounds, "seek would give an invalid file offset")                                                           \
	XX(NoMoreFiles, "readdir has no more files to return")                                                             \
	XX(OutOfFileDescs, "Cannot open another file until one is closed")

enum class Value {
#define XX(tag, text) tag,
	IFS_ERROR_MAP(XX)
#undef XX
		MAX, // Mark end of value range
};

// Define the Error::xxx codes as negative versions of the enumerated values
#define XX(tag, text) constexpr int tag{-int(Value::tag)};
IFS_ERROR_MAP(XX)
#undef XX

constexpr ErrorCode USER{-100};	// Start of user-defined codes
constexpr ErrorCode SYSTEM{-1000}; // Non-FWFS filing systems map custom codes starting here

/**
 * @brief get text for an error code
 * @param err
 * @retval String
 */
String toString(int err);

/**
 * @brief Determine if the given IFS error code is system-specific
 */
inline bool isSystem(int err)
{
	return err <= SYSTEM;
}

/**
 * @brief Translate system error code into IFS error code
 */
inline int fromSystem(int syscode)
{
	return (syscode < 0) ? SYSTEM + syscode : syscode;
}

/**
 * @brief Translate IFS error code into SYSTEM code
 */
inline int toSystem(int err)
{
	return isSystem(err) ? (err - SYSTEM) : err;
}

} // namespace Error

constexpr ErrorCode FS_OK = Error::Success;

} // namespace IFS
