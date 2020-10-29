/*
 * FileDefs.cpp
 *
 *  Created on: 26 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/Object.h"
#include <FlashString/Vector.hpp>

namespace IFS
{
constexpr uint8_t Object::FWOBT_REF;
}

namespace
{
#define XX(value, tag, text) DEFINE_FSTR_LOCAL(str_##tag, #tag)
FWFS_OBJTYPE_MAP(XX)
#undef XX

#define XX(value, tag, text) &str_##tag,
DEFINE_FSTR_VECTOR_LOCAL(typeStrings, FSTR::String, FWFS_OBJTYPE_MAP(XX))
#undef XX

} // namespace

String toString(IFS::Object::Type obt)
{
	String s = typeStrings[unsigned(obt)];
	if(!s) {
		// Custom object type?
		s = '#';
		s += unsigned(obt);
	}

	return s;
}
