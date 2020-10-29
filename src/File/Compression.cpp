/*
 * Compression.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "../include/IFS/File/Compression.h"
#include <FlashString/String.hpp>
#include <FlashString/Vector.hpp>

namespace
{
#define XX(tag, comment) DEFINE_FSTR_LOCAL(str_##tag, #tag)
COMPRESSION_TYPE_MAP(XX)
#undef XX

#define XX(tag, comment) &str_##tag,
DEFINE_FSTR_VECTOR_LOCAL(strings, FSTR::String, COMPRESSION_TYPE_MAP(XX))
#undef XX

} // namespace

String toString(IFS::File::Compression type)
{
	return strings[unsigned(type)];
}
