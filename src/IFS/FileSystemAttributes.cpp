/*
 * FileSystemAttributes.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "FileSystemAttributes.h"
#include "flags.h"

#define XX(_tag, _comment) static DEFINE_PSTR(attrstr_##_tag, #_tag)
FILE_SYSTEM_ATTR_MAP(XX)
#undef XX

#define XX(_tag, _comment) attrstr_##_tag,
static PGM_P const attributeStrings[] PROGMEM = {FILE_SYSTEM_ATTR_MAP(XX)};
#undef XX

char* fileSystemAttrToStr(FileSystemAttributes attr, char* buf, size_t bufSize)
{
	return flagsToStr(attr, attributeStrings, ARRAY_SIZE(attributeStrings), buf, bufSize);
}
