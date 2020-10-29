/*
 * Access.cpp
 *
 *  Created on: 6 Jun 2018
 *      Author: mikee47
 *
 * Access control definitions
 */

#include "../include/IFS/File/Access.h"

String toString(const IFS::File::ACL& acl)
{
	String s;
	s += IFS::getChar(acl.readAccess);
	s += IFS::getChar(acl.writeAccess);
	return s;
}
