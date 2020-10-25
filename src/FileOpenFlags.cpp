/*
 * FileOpenFlags.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/FileOpenFlags.h"
#include "Flags.h"

namespace IFS
{
#define XX(_tag, _comment) DEFINE_PSTR_LOCAL(flagstr_##_tag, #_tag)
FILE_OPEN_FLAG_MAP(XX)
#undef XX

#define XX(_tag, _comment) flagstr_##_tag,
static PGM_P const flagStrings[] PROGMEM = {FILE_OPEN_FLAG_MAP(XX)};
#undef XX

char* fileOpenFlagsToStr(FileOpenFlags flags, char* buf, size_t bufSize)
{
	return flagsToStr(flags, flagStrings, ARRAY_SIZE(flagStrings), buf, bufSize);
}

} // namespace IFS
