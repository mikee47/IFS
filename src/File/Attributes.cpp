/*
 * Attributes.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "../include/IFS/File/Attributes.h"
#include <FlashString/Array.hpp>

namespace IFS
{
#define XX(tag, ch, comment) #ch
DEFINE_FSTR_LOCAL(attributeChars, FILEATTR_MAP(XX))
#undef XX

String toString(File::Attributes attr)
{
	String s = attributeChars;

	for(unsigned a = 0; a < s.length(); ++a) {
		if(!attr[File::Attribute(a)]) {
			s[a] = '.';
		}
	}

	return s;
}

} // namespace IFS
