/*
 * FileSystemAttributes.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/FileSystem.h"
#include "Flags.h"

namespace IFS
{
#define XX(_name, _tag, _desc) DEFINE_PSTR_LOCAL(typestr_##_name, #_tag)
FILESYSTEM_TYPE_MAP(XX)
#undef XX

static PGM_P const typeStrings[] PROGMEM = {
#define XX(_name, _tag, _desc) typestr_##_name,
	FILESYSTEM_TYPE_MAP(XX)
#undef XX
};

#define XX(_tag, _comment) DEFINE_PSTR_LOCAL(attrstr_##_tag, #_tag)
FILE_SYSTEM_ATTR_MAP(XX)
#undef XX

#define XX(_tag, _comment) attrstr_##_tag,
static PGM_P const attributeStrings[] PROGMEM = {FILE_SYSTEM_ATTR_MAP(XX)};
#undef XX

char* toString(IFileSystem::Attributes attr, char* buf, size_t bufSize)
{
	return flagsToStr(attr.getValue(), attributeStrings, ARRAY_SIZE(attributeStrings), buf, bufSize);
}

char* toString(IFileSystem::Type type, char* buf, unsigned bufSize)
{
	if(type >= IFileSystem::Type::MAX) {
		type = IFileSystem::Type::Unknown;
	}
	return strncpy_P(buf, typeStrings[(unsigned)type], bufSize);
}

} // namespace IFS
