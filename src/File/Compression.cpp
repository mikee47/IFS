/*
 * Compression.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "../include/IFS/File/Compression.h"

namespace IFS
{
#define XX(_tag, _comment) DEFINE_PSTR_LOCAL(__str_##_tag, #_tag)
COMPRESSION_TYPE_MAP(XX)
#undef XX

#define XX(_tag, _comment) __str_##_tag,
static PGM_P const __strings[] PROGMEM = {COMPRESSION_TYPE_MAP(XX)};
#undef XX

char* toString(File::Compression type, char* buf, size_t bufSize)
{
	if(buf && bufSize) {
		if(type < File::Compression::MAX) {
			strncpy_P(buf, __strings[(unsigned)type], bufSize);
			buf[bufSize - 1] = '\0';
		} else {
			buf[0] = '\0';
		}
	}
	return buf;
}

} // namespace IFS
