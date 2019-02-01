/*
 * flags.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "flags.h"

char* flagsToStr(uint32_t flags, PGM_P const* strings, unsigned flagCount, char* buf, size_t bufSize)
{
	size_t off = 0;
	for(unsigned f = 0; f < flagCount; ++f) {
		if(!bitRead(flags, f)) {
			continue;
		}

		if(off && (off + 2) < bufSize) {
			buf[off++] = ',';
			buf[off++] = ' ';
		}

		PGM_P pstr = strings[f];
		int len = strlen_P(pstr);
		if(off + len > bufSize) {
			len = bufSize - off;
		}
		memcpy_P(&buf[off], pstr, len);
		off += len;
	}

	buf[off] = '\0';
	return buf;
}
