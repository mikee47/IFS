/*
 * Compression.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/Compression.h"
#include <FlashString/String.hpp>
#include <FlashString/Vector.hpp>

namespace
{
#define XX(tag, comment) DEFINE_FSTR_LOCAL(str_##tag, #tag)
IFS_COMPRESSION_TYPE_MAP(XX)
#undef XX

#define XX(tag, comment) &str_##tag,
DEFINE_FSTR_VECTOR_LOCAL(strings, FSTR::String, IFS_COMPRESSION_TYPE_MAP(XX))
#undef XX

} // namespace

namespace IFS
{
Compression::Type getCompressionType(const char* str, Compression::Type defaultValue)
{
	int i = strings.indexOf(str);
	return (i < 0) ? defaultValue : Compression::Type(i);
}

} // namespace IFS

String toString(IFS::Compression::Type type)
{
	String s = strings[unsigned(type)];
	return s ?: F("UNK#") + String(unsigned(type));
}
