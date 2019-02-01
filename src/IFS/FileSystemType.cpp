/*
 * FileSystemType.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "FileSystemType.h"

#define XX(_name, _tag, _desc) static DEFINE_PSTR(typestr_##_name, #_tag)
FILESYSTEM_TYPE_MAP(XX)
#undef XX

static PGM_P const typeStrings[] PROGMEM = {
#define XX(_name, _tag, _desc) typestr_##_name,
	FILESYSTEM_TYPE_MAP(XX)
#undef XX
};

char* fileSystemTypeToStr(FileSystemType type, char* buf, unsigned bufSize)
{
	if(type >= FileSystemType::MAX)
		type = FileSystemType::Unknown;
	return strncpy_P(buf, typeStrings[(unsigned)type], bufSize);
}
