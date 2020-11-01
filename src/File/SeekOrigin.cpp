/*
 * SeekOrigin.h
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include <IFS/File/SeekOrigin.h>
#include <FlashString/Vector.hpp>

namespace
{
#define XX(tag, value, comment) DEFINE_FSTR_LOCAL(str_##tag, #tag)
FILE_SEEK_ORIGIN_MAP(XX)
#undef XX

#define XX(tag, value, comment) &str_##tag,
DEFINE_FSTR_VECTOR_LOCAL(strings, FSTR::String, FILE_SEEK_ORIGIN_MAP(XX))
#undef XX

} // namespace

String toString(IFS::File::SeekOrigin origin)
{
	return strings[unsigned(origin)];
}
