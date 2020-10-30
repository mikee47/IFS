/*
 * File::OpenFlags.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "../include/IFS/File/OpenFlags.h"
#include "../Flags.h"

namespace IFS
{
#define XX(tag, comment) DEFINE_FSTR_LOCAL(flagstr_##tag, #tag)
FILE_OPEN_FLAG_MAP(XX)
#undef XX

#define XX(tag, comment) &flagstr_##tag,
DEFINE_FSTR_VECTOR_LOCAL(flagStrings, FSTR::String, FILE_OPEN_FLAG_MAP(XX))
#undef XX

String toString(File::OpenFlags flags)
{
	return flagsToStr(flags.getValue(), flagStrings);
}

} // namespace IFS
