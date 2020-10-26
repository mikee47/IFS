/*
 * Access.cpp
 *
 *  Created on: 6 Jun 2018
 *      Author: mikee47
 *
 * Access control definitions
 */

#include "../include/IFS/File/Access.h"

namespace IFS
{
char* toString(const File::ACL& acl, char* buf, size_t bufSize)
{
	if(buf && bufSize) {
		if(bufSize < 3)
			buf[0] = '\0';
		else {
			buf[0] = userRoleChar(acl.readAccess);
			buf[1] = userRoleChar(acl.writeAccess);
			buf[2] = '\0';
		}
	}
	return buf;
}

} // namespace IFS
