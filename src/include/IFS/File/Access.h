/*
 * Access.h
 *
 *  Created on: 6 Jun 2018
 *      Author: mikee47
 */

#pragma once

#include "../UserRole.h"

namespace IFS
{
namespace File
{
/*
 * Role-based Access Control List.
 *
 * We only require two entries to explicitly define read/write access.
 */
struct ACL {
	/* Minimum access permissions */
	UserRole readAccess : 8;
	UserRole writeAccess : 8;
};

} // namespace File

char* toString(const File::ACL& acl, char* buf, size_t bufSize);

} // namespace IFS
