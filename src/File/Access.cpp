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
String toString(const File::ACL& acl)
{
	String s;
	s += getChar(acl.readAccess);
	s += getChar(acl.writeAccess);
	return s;
}

} // namespace IFS
