/**
 * UserRole.h
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

#include "Types.h"

namespace IFS
{
// Access Control level
#define IFS_USER_ROLE_MAP(XX)                                                                                          \
	XX(None, -, "No assigned role")                                                                                    \
	XX(Guest, g, "User-type access without authentication")                                                            \
	XX(User, u, "Normal user")                                                                                         \
	XX(Manager, m, "Perform restricted system functions, reset user passwords, etc.")                                  \
	XX(Admin, a, "Full access")

enum class UserRole : uint8_t {
#define XX(_tag, _char, _comment) _tag,
	IFS_USER_ROLE_MAP(XX)
#undef XX
		MAX ///< Actually maxmimum value + 1...
};

/**
 * @name Return the access type value for the given string.
 * @param str
 * @param defaultRole Returned if string isn't recognsed
 * @retval UserRole
 * @{
 */
UserRole getUserRole(const char* str, UserRole defaultRole);

inline UserRole getUserRole(const String& str, UserRole defaultRole)
{
	return getUserRole(str.c_str(), defaultRole);
}
/** @} */

/*
 * @brief Get the character code representing the given access type
 * @param role
 * @retval char
 */
char getChar(UserRole role);

/*
 * @brief Return the access type corresponding to the given code.
 * @param code
 * @param defaultRole Returned if code isn't recognised
 * @retval UserRole
 */
UserRole getUserRole(char code, UserRole defaultRole);

} // namespace IFS

/*
 * Get the string representation for the given access type.
 */
String toString(IFS::UserRole role);
