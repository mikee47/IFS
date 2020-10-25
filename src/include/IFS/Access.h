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
	XX(None, '-', "No assigned role")                                                                                  \
	XX(Guest, 'g', "User-type access without authentication")                                                          \
	XX(User, 'u', "Normal user")                                                                                       \
	XX(Manager, 'm', "Perform restricted system functions, reset user passwords, etc.")                                \
	XX(Admin, 'a', "Full access")

enum class UserRole : uint8_t {
#define XX(_tag, _char, _comment) _tag,
	USER_ROLE_MAP(XX)
#undef XX
		MAX ///< Actually maxmimum value + 1...
};

/*
 * Get the string representation for the given access type.
 */
char* userRoleToStr(UserRole role, char* buf, size_t bufSize);

/*
 * Return the access type value for the given string.
 * If the string isn't recognised, return the given default value.
 */
UserRole getUserRole(const char* str, UserRole _default);

/*
 * Get the character code representing the given access type.
 */
char userRoleChar(UserRole access);

/*
 * Return the access type corresponding to the given code.
 * If the code isn't valid, return the given default.
 */
UserRole getUserRole(char c, UserRole _default);

/*
 * Role-based Access Control List.
 *
 * We only require two entries to explicitly define read/write access.
 */
struct FileACL {
	/* Minimum access permissions */
	UserRole readAccess : 8;
	UserRole writeAccess : 8;
};

char* fileAclToStr(FileACL acl, char* buf, size_t bufSize);

} // namespace IFS
