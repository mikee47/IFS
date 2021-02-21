/*
 * OpenFlags.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/OpenFlags.h"
#include <FlashString/Vector.hpp>

namespace
{
#define XX(tag, comment) DEFINE_FSTR_LOCAL(flagstr_##tag, #tag)
IFS_OPEN_FLAG_MAP(XX)
#undef XX

#define XX(tag, comment) &flagstr_##tag,
DEFINE_FSTR_VECTOR_LOCAL(flagStrings, FSTR::String, IFS_OPEN_FLAG_MAP(XX))
#undef XX

} // namespace

String toString(IFS::OpenFlag flag)
{
	return flagStrings[unsigned(flag)];
}
