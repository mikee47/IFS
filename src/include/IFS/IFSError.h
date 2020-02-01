/****
 * IFS Error codes and related stuff
 *
 * Created August 2018 by mikee471
 *
 ****/

/**	@defgroup filesystem File system
 *	@brief	Installable file system
 *  @{
 */

#pragma once

/** @brief IFS return codes
 *  @note An IFS implementation must return negative values for errors. Wrappers may
 *  use these IFS codes instead of their own.
 *  If returning an internal error code it may need to be translated.
 *  Methods returning ONLY an error code (i.e. not file_t) may return positive 'error'
 *  codes for information purposes. See IFileSystem::check() as an example.
 *  Return value usage is consistent with SPIFFS.
 *
 *  These codes are allocated using an enum since they are purely for internal purposes.
 */

#define FSERROR_MAP(XX)                                                                                                \
	XX(OK, "Success")                                                                                                  \
	XX(NoFileSystem, "File system has not been set")                                                                   \
	XX(NoMedia, "File system has no media object")                                                                     \
	XX(BadStore, "Store is invalid")                                                                                   \
	XX(StoreNotMounted, "Store not mounted")                                                                           \
	XX(NotMounted, "File system is not mounted")                                                                       \
	XX(BadObject, "Malformed filesystem object")                                                                       \
	XX(BadFileSystem, "File system corruption detected")                                                               \
	XX(BadParam, "Invalid parameter(s)")                                                                               \
	XX(BadExtent, "Invalid media extent")                                                                              \
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

enum IFSError {
#define XX(_tag, _text) eFSERR_##_tag,
	FSERROR_MAP(XX)
#undef XX
		eFSERR_MAX // Mark end of value range
};

// Success code
constexpr int FS_OK = eFSERR_OK;

// Define the FSERR_xxx codes as negative versions of the enumerated values
#define XX(_tag, _text) const int FSERR_##_tag = -eFSERR_##_tag;
FSERROR_MAP(XX)
#undef XX

// Start of user-defined error codes
constexpr int FSERR_USER = -64;

/** @brief get text for an error code
 *  @param err
 *  @param buffer
 *  @param size
 *  @retval length of message, excluding nul terminator
 *  @note message is always nul terminated
 */
int fsGetErrorText(int err, char* buffer, unsigned size);
