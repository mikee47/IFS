/*
 * Error.cpp
 *
 *  Created on: 19 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/Error.h"
#include "include/IFS/Types.h"

namespace IFS
{
namespace Error
{
/* @brief Define string table for standard IFS error codes
 *
 */
#define XX(_tag, _text) DEFINE_PSTR_LOCAL(FSES_##_tag, #_tag)
IFS_ERROR_MAP(XX)
#undef XX

static PGM_P const errorStrings[] PROGMEM = {
#define XX(_tag, _text) FSES_##_tag,
	IFS_ERROR_MAP(XX)
#undef XX
};

int toString(int err, char* buffer, unsigned size)
{
	if(buffer == nullptr || size == 0) {
		return Error::BadParam;
	}

	if(err > 0) {
		err = 0;
	} else {
		err = -err;
	}

	if(err >= Error::USER) {
		return snprintf(buffer, size, _F("FSERR #%u"), err);
	}

	strncpy_P(buffer, errorStrings[err], size);
	buffer[size - 1] = '\0';
	return strlen(buffer);
}

} // namespace Error
} // namespace IFS
