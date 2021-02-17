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

namespace IFS
{
namespace File
{
File::Compression::Type getCompressionType(const char* str, File::Compression::Type defaultValue)
{
	int i = strings.indexOf(str);
	return (i < 0) ? defaultValue : File::Compression::Type(i);
}

} // namespace File
} // namespace IFS

String toString(IFS::File::Compression::Type type)
{
	String s = strings[unsigned(type)];
	return s ?: F("UNK#") + String(unsigned(type));
}
