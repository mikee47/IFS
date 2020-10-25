/*
 * FileDefs.cpp
 *
 *  Created on: 26 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/Object.h"

namespace IFS
{
#define XX(_value, _tag, _text) DEFINE_PSTR(__str_##_tag, #_tag)
FWFS_OBJTYPE_MAP(XX)
#undef XX

char* toString(Object::Type obt, char* buffer, size_t bufSize)
{
	if(buffer == nullptr || bufSize == 0) {
		return buffer;
	}

#define XX(value, tag, text)                                                                                           \
	if(obt == Object::Type::tag)                                                                                       \
		strncpy_P(buffer, __str_##tag, bufSize);                                                                       \
	else
	FWFS_OBJTYPE_MAP(XX)
#undef XX
	snprintf(buffer, bufSize, _F("#%u"), obt);

	buffer[bufSize - 1] = '\0';
	return buffer;
}

} // namespace IFS
