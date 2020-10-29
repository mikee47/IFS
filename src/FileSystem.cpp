/*
 * FileSystemAttributes.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/FileSystem.h"
#include <FlashString/Vector.hpp>

namespace IFS
{
#define XX(name, tag, desc) DEFINE_FSTR_LOCAL(typestr_##name, #tag)
FILESYSTEM_TYPE_MAP(XX)
#undef XX

#define XX(name, tag, desc) &typestr_##name,
DEFINE_FSTR_VECTOR_LOCAL(typeStrings, FSTR::String, FILESYSTEM_TYPE_MAP(XX))
#undef XX

String toString(IFileSystem::Type type)
{
	if(type >= IFileSystem::Type::MAX) {
		type = IFileSystem::Type::Unknown;
	}
	return typeStrings[(unsigned)type];
}
#define XX(tag, comment) DEFINE_FSTR_LOCAL(attrstr_##tag, #tag)
FILE_SYSTEM_ATTR_MAP(XX)
#undef XX

#define XX(tag, comment) &attrstr_##tag,
DEFINE_FSTR_VECTOR_LOCAL(attributeStrings, FSTR::String, FILE_SYSTEM_ATTR_MAP(XX))
#undef XX

String toString(IFileSystem::Attribute attr)
{
	return attributeStrings[unsigned(attr)];
}

} // namespace IFS
