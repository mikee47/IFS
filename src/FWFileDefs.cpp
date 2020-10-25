/*
 * FWFileDefs.cpp
 *
 *  Created on: 26 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/FWFileDefs.h"
#include "IFS/Types.h"

namespace IFS
{
#define XX(_value, _tag, _text) DEFINE_PSTR(__str_##_tag, #_tag)
FWFS_OBJTYPE_MAP(XX)
#undef XX

char* fwfsObjectTypeName(FWFS_ObjectType obt, char* buffer, size_t bufSize)
{
	if(buffer == nullptr || bufSize == 0) {
		return buffer;
	}

#define XX(_value, _tag, _text)                                                                                        \
	if(obt == fwobt_##_tag)                                                                                            \
		strncpy_P(buffer, __str_##_tag, bufSize);                                                                      \
	else
	FWFS_OBJTYPE_MAP(XX)
#undef XX
	snprintf(buffer, bufSize, _F("#%u"), obt);

	buffer[bufSize - 1] = '\0';
	return buffer;
}

} // namespace IFS
