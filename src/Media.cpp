/*
 * Media.cpp
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/Media.h"
#include "include/IFS/Error.h"
#include <FlashString/Vector.hpp>

namespace IFS
{
constexpr Media::Attributes Media::ReadWrite;
constexpr Media::Attributes Media::ReadOnly;

int Media::readname(char* buffer, unsigned bufsize, uint32_t offset, unsigned len)
{
	if(buffer == nullptr || bufsize == 0) {
		return Error::BadParam;
	}

	int err = FS_OK;
	if(len >= bufsize) {
		len = bufsize - 1;
		err = Error::NameTooLong;
	}

	int res = len ? read(offset, len, buffer) : FS_OK;
	buffer[len] = '\0';
	return res < 0 ? res : err;
}

#define XX(tag, comment) DEFINE_FSTR_LOCAL(typestr_##tag, #tag)
IFS_MEDIA_TYPE_MAP(XX)
#undef XX

#define XX(tag, comment) &typestr_##tag,
DEFINE_FSTR_VECTOR_LOCAL(typeStrings, FSTR::String, IFS_MEDIA_TYPE_MAP(XX))
#undef XX

String toString(Media::Type type)
{
	return typeStrings[unsigned(type)];
}

#define XX(tag, comment) DEFINE_FSTR_LOCAL(busstr_##tag, #tag)
IFS_MEDIA_BUS_MAP(XX)
#undef XX

#define XX(tag, comment) &busstr_##tag,
DEFINE_FSTR_VECTOR_LOCAL(busStrings, FSTR::String, IFS_MEDIA_BUS_MAP(XX))
#undef XX

String toString(Media::Bus bus)
{
	return busStrings[unsigned(bus)];
}

#define XX(tag, comment) DEFINE_FSTR_LOCAL(attrstr_##tag, #tag)
IFS_MEDIA_ATTRIBUTE_MAP(XX)
#undef XX

#define XX(tag, comment) &attrstr_##tag,
DEFINE_FSTR_VECTOR_LOCAL(attrStrings, FSTR::String, IFS_MEDIA_ATTRIBUTE_MAP(XX))
#undef XX

String toString(Media::Attribute attr)
{
	return attrStrings[unsigned(attr)];
}

} // namespace IFS
