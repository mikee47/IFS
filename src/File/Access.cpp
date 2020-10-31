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
namespace File
{
String getAclString(const IFS::File::ACL& acl)
{
	String s;
	s += IFS::getChar(acl.readAccess);
	s += IFS::getChar(acl.writeAccess);
	return s;
}

} // namespace File
} // namespace IFS

String toString(const IFS::File::ACL& acl)
{
	String s;
	s += toString(acl.readAccess);
	s += '/';
	s += toString(acl.writeAccess);
	return s;
}
