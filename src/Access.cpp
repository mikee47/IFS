/*
 * access.cpp
 *
 *  Created on: 6 Jun 2018
 *      Author: mikee47
 *
 * Access control definitions
 */

#include "include/IFS/Access.h"

namespace IFS
{
#define XX(_tag, _char, _comment) DEFINE_PSTR_LOCAL(accstr_##_tag, #_tag)
USER_ROLE_MAP(XX)
#undef XX

#define XX(_tag, _char, _comment) accstr_##_tag,
static PGM_P const accessStrings[] PROGMEM = {USER_ROLE_MAP(XX)};
#undef XX

#define XX(_tag, _char, _comment) _char,
static DEFINE_PSTR(accessChars, {USER_ROLE_MAP(XX)});
#undef XX

char* userRoleToStr(UserRole role, char* buf, size_t bufSize)
{
	if(buf && bufSize) {
		if(role < UserRole::MAX) {
			strncpy_P(buf, accessStrings[(unsigned)role], bufSize);
			buf[bufSize - 1] = '\0';
		} else
			buf[0] = '\0';
	}
	return buf;
}

UserRole getUserRole(const char* str, UserRole _default)
{
	for(unsigned i = 0; i < ARRAY_SIZE(accessStrings); ++i) {
		if(strcasecmp_P(str, accessStrings[i]) == 0) {
			return UserRole(i);
		}
	}

	return _default;
}

char userRoleChar(UserRole role)
{
	LOAD_PSTR(chars, accessChars);
	// Return 'x' if unknown
	return (role < UserRole::MAX) ? chars[(unsigned)role] : 'x';
}

UserRole getUserRole(char c, UserRole _default)
{
#define XX(_tag, _char, _comment)                                                                                      \
	if(c == _char)                                                                                                     \
		return UserRole::_tag;
	USER_ROLE_MAP(XX)
#undef XX
	return _default;
}

char* fileAclToStr(FileACL acl, char* buf, size_t bufSize)
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
