/**
 * HostUtil.cpp
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

#include "include/IFS/Host/Util.h"
#include <fcntl.h>

#include <string.h>
extern "C" char* strerror_r(int, char*, size_t);

#ifndef O_BINARY
#define O_BINARY 0
#endif

namespace IFS::Host
{
int mapFlags(OpenFlags flags)
{
	int ret = O_BINARY;
	if(flags[OpenFlag::Append]) {
		ret |= O_APPEND;
	}
	if(flags[OpenFlag::Create]) {
		ret |= O_CREAT;
	}
	if(flags[OpenFlag::Truncate]) {
		ret |= O_TRUNC;
	}
	if(flags[OpenFlag::Read]) {
		if(flags[OpenFlag::Write]) {
			ret |= O_RDWR;
		} else {
			ret |= O_RDONLY;
		}
	} else if(flags[OpenFlag::Write]) {
		ret |= O_WRONLY;
	}
	return ret;
}

String getErrorString(int err)
{
	if(Error::isSystem(err)) {
		char buffer[256]{};
		auto r = strerror_r(-Error::toSystem(err), buffer, sizeof(buffer));
		/*
		 * There's no guarantee which version of strerror_r got linked,
		 * but the return value gives us a clue
		 */
		if(int(r) == -1 || uint32_t(r) < 0x10000) {
			return buffer;
		} else {
			return r;
		}
	}

	return IFS::Error::toString(err);
}

} // namespace IFS::Host
