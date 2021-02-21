/*
 * Access.h
 *
 *  Created on: 6 Jun 2018
 *      Author: mikee47
 */

#pragma once

#include "UserRole.h"

namespace IFS
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

	bool operator==(const ACL& other) const
	{
		return other.readAccess == readAccess && other.writeAccess == writeAccess;
	}

	bool operator!=(const ACL& other) const
	{
		return !operator==(other);
	}
};

/**
 * @brief Return a brief textual representation for an ACL
 * Suitable for inclusion in a file listing
 * @param acl
 * @retval String
 */
String getAclString(const IFS::ACL& acl);

} // namespace IFS

/**
 * @brief Return a descriptive textual representation for an ACL
 * @param acl
 * @retval String
 */
String toString(const IFS::ACL& acl);
