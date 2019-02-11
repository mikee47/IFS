/*
 * IFSError.cpp
 *
 *  Created on: 19 Aug 2018
 *      Author: mikee47
 */

#include "IFSError.h"
#include "ifstypes.h"

/* @brief Define string table for standard IFS error codes
 *
 */
#define XX(_tag, _text) static DEFINE_PSTR(FSES_##_tag, #_tag)
FSERROR_MAP(XX)
#undef XX

static PGM_P const errorStrings[] PROGMEM = {
#define XX(_tag, _text) FSES_##_tag,
	FSERROR_MAP(XX)
#undef XX
};

int fsGetErrorText(int err, char* buffer, unsigned size)
{
	if(buffer == nullptr || size == 0) {
		return FSERR_BadParam;
	}

	if(err > 0) {
		err = 0;
	} else {
		err = -err;
	}

	if(err >= eFSERR_MAX) {
		return snprintf(buffer, size, _F("FSERR #%u"), err);
	}

	strncpy_P(buffer, errorStrings[err], size);
	buffer[size - 1] = '\0';
	return strlen(buffer);
}
