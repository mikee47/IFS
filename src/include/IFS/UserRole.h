/*
 * Access.h
 *
 *  Created on: 6 Jun 2018
 *      Author: mikee47
 */

#pragma once

#include "Types.h"

namespace IFS
{
// Access Control level
#define USER_ROLE_MAP(XX)                                                                                              \
	XX(None, -, "No assigned role")                                                                                    \
	XX(Guest, g, "User-type access without authentication")                                                            \
	XX(User, u, "Normal user")                                                                                         \
	XX(Manager, m, "Perform restricted system functions, reset user passwords, etc.")                                  \
	XX(Admin, a, "Full access")

enum class UserRole : uint8_t {
#define XX(_tag, _char, _comment) _tag,
	USER_ROLE_MAP(XX)
#undef XX
		MAX ///< Actually maxmimum value + 1...
};

/*
 * Get the string representation for the given access type.
 */
String toString(UserRole role);

/*
 * @brief Return the access type value for the given string.
 * @param str
 * @param defaultRole Returned if string isn't recognsed
 * @retval UserRole
 */
UserRole getUserRole(const char* str, UserRole defaultRole);

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
