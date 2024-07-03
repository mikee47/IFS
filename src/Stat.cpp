/**
 * Stat.cpp
 *
 * Copyright 2022 mikee47 <mike@sillyhouse.net>
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

#include "include/IFS/Stat.h"
#include "include/IFS/FileSystem.h"
#include <Print.h>

namespace IFS
{
size_t Stat::printTo(Print& p) const
{
	if(fs == nullptr) {
		return 0;
	}

	FileSystem::Info info;
	fs->getinfo(info);

	size_t n{0};
	n += p.print(String(name).padRight(50));
	n += p.print(' ');
	n += p.print(size, DEC, 8, ' ');
	n += p.print(' ');
	n += p.print(info.type);
	n += p.print(" #0x");
	n += p.print(id, HEX, 8);
	n += p.print(' ');
	n += p.print(mtime.toString());
	n += p.print(' ');
	n += p.print(acl);
	n += p.print(" {");
	n += p.print(compression.type);
	n += p.print('}');
	return n;
}

} // namespace IFS
