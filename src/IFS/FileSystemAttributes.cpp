/*
 * FileSystemAttributes.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "FileSystemAttributes.h"
#include "flags.h"

#define XX(_tag, _comment) static DEFINE_PSTR(__str_##_tag, #_tag)
FILE_SYSTEM_ATTR_MAP(XX)
#undef XX

#define XX(_tag, _comment) __str_##_tag,
static PGM_P const __strings[] PROGMEM = {FILE_SYSTEM_ATTR_MAP(XX)};
#undef XX

char* fileSystemAttrToStr(FileSystemAttributes attr, char* buf, size_t bufSize)
{
	return flagsToStr(attr, __strings, ARRAY_SIZE(__strings), buf, bufSize);
}
