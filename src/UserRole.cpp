/**
 * UserRole.cpp
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

#include "include/IFS/UserRole.h"
#include <FlashString/Vector.hpp>

namespace
{
#define XX(tag, char, comment) DEFINE_FSTR_LOCAL(str_##tag, #tag)
IFS_USER_ROLE_MAP(XX)
#undef XX

#define XX(tag, char, comment) &str_##tag,
DEFINE_FSTR_VECTOR_LOCAL(userRoleStrings, FSTR::String, IFS_USER_ROLE_MAP(XX))
#undef XX

#define XX(tag, ch, comment) #ch
DEFINE_FSTR_LOCAL(userRoleChars, IFS_USER_ROLE_MAP(XX))
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
