/*
 * Media.cpp
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/Media.h"
#include "include/IFS/Error.h"

namespace IFS
{
int Media::readname(char* buffer, unsigned bufsize, uint32_t offset, unsigned len)
{
	if(buffer == nullptr || bufsize == 0) {
		return Error::BadParam;
	}

	int err = FS_OK;
	if(len >= bufsize) {
		len = bufsize - 1;
		err = Error::NameTooLong;
	}

	int res = len ? read(offset, len, buffer) : FS_OK;
	buffer[len] = '\0';
	return res < 0 ? res : err;
}

} // namespace IFS
