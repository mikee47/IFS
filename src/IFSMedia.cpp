/*
 * IFSMedia.cpp
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 */

#include <IFS/IFSMedia.h>
#include <IFS/IFSError.h>

int IFSMedia::readname(char* buffer, unsigned bufsize, uint32_t offset, unsigned len)
{
	if(buffer == nullptr || bufsize == 0) {
		return FSERR_BadParam;
	}

	int err = FS_OK;
	if(len >= bufsize) {
		len = bufsize - 1;
		err = FSERR_NameTooLong;
	}

	int res = len ? read(offset, len, buffer) : FS_OK;
	buffer[len] = '\0';
	return res < 0 ? res : err;
}
