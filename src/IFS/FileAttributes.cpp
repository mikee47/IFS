/*
 * FileAttr.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "FileAttributes.h"

static const char __chars[] PROGMEM = {
#define XX(_tag, _char, _comment) _char,
	FILEATTR_MAP(XX)
#undef XX
};

char* fileAttrToStr(FileAttributes attr, char* buf, size_t bufSize)
{
	size_t len = 0;

	LOAD_PSTR(chars, __chars);
	for(unsigned a = 0; a < ARRAY_SIZE(__chars); ++a) {
		if(len >= bufSize)
			break;
		buf[len++] = bitRead(attr, a) ? chars[a] : '.';
	}

	if(len < bufSize)
		buf[len] = '\0';
	else if(bufSize)
		buf[bufSize - 1] = '\0';
	return buf;
}
