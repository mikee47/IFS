/**
 * Access.h
 * Access control definitions
 *
 * Created on: 6 Jun 2018
 *
 * Copyright 2019 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the IFS Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ****/

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
