/*
 * Error.cpp
 *
 *  Created on: 19 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/Error.h"
#include "include/IFS/Types.h"
#include <FlashString/String.hpp>
#include <FlashString/Vector.hpp>

namespace IFS
{
namespace Error
{
/*
 * Define string table for standard IFS error codes
 */
#define XX(tag, text) DEFINE_FSTR_LOCAL(FS_##tag, #tag)
IFS_ERROR_MAP(XX)
#undef XX

#define XX(tag, text) &FS_##tag,
DEFINE_FSTR_VECTOR_LOCAL(errorStrings, FlashString, IFS_ERROR_MAP(XX))
#undef XX

String toString(int err)
{
	if(err > 0) {
		err = 0;
	} else {
		err = -err;
	}

	String s = errorStrings[err];
	if(!s) {
		String s = F("FSERR #");
		s += err;
	}

	return s;
}

} // namespace Error
} // namespace IFS
