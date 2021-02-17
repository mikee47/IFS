/*
 * UserRole.cpp
 *
 *  Created on: 6 Jun 2018
 *      Author: mikee47
 *
 * User roles used for access control
 */

#include "include/IFS/UserRole.h"
#include <FlashString/Vector.hpp>

namespace
{
#define XX(tag, char, comment) DEFINE_FSTR_LOCAL(str_##tag, #tag)
USER_ROLE_MAP(XX)
#undef XX

#define XX(tag, char, comment) &str_##tag,
DEFINE_FSTR_VECTOR(userRoleStrings, FSTR::String, USER_ROLE_MAP(XX))
#undef XX

#define XX(tag, ch, comment) #ch
DEFINE_FSTR_LOCAL(userRoleChars, USER_ROLE_MAP(XX))
#undef XX
} // namespace

namespace IFS
{
UserRole getUserRole(const char* str, UserRole defaultRole)
{
	int i = userRoleStrings.indexOf(str);
	return (i < 0) ? defaultRole : UserRole(i);
}

char getChar(UserRole role)
{
	String s = userRoleChars;
	// Return 'x' if unknown
	return (role < UserRole::MAX) ? s[(unsigned)role] : 'x';
}

UserRole getUserRole(char code, UserRole defaultRole)
{
	String s = userRoleChars;
	int i = s.indexOf(code);
	return (i < 0) ? defaultRole : UserRole(i);
}

} // namespace IFS

String toString(IFS::UserRole role)
{
	String s = userRoleStrings[unsigned(role)];
	return s ?: F("UNK#") + String(unsigned(role));
}
