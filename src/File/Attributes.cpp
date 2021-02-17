/*
 * Attributes.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "../include/IFS/File/Attributes.h"
#include <FlashString/Vector.hpp>

namespace
{
#define XX(tag, ch, comment) #ch
DEFINE_FSTR_LOCAL(attributeChars, FILEATTR_MAP(XX))
#undef XX

#define XX(tag, ch, comment) DEFINE_FSTR_LOCAL(attstr_##tag, #tag)
FILEATTR_MAP(XX)
#undef XX

#define XX(tag, ch, comment) &attstr_##tag,
DEFINE_FSTR_VECTOR_LOCAL(attributeStrings, FSTR::String, FILEATTR_MAP(XX))
#undef XX

} // namespace

namespace IFS
{
namespace File
{
String getAttributeString(File::Attributes attr)
{
	String s = attributeChars;

	for(unsigned a = 0; a < s.length(); ++a) {
		if(!attr[File::Attribute(a)]) {
			s[a] = '.';
		}
	}

	return s;
}

} // namespace File
} // namespace IFS

String toString(IFS::File::Attribute attr)
{
	String s = attributeStrings[unsigned(attr)];
	return s ?: F("UNK#") + String(unsigned(attr));
}
