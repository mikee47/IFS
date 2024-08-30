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
#include <cstring>

#ifndef O_BINARY
#define O_BINARY 0
#endif

namespace
{
/*
 * There's no guarantee which version of strerror_r got linked,
 * so use overloaded helper function to get correct result.
 */
[[maybe_unused]] const char* get_error_string(char* retval, char*)
{
	return retval;
}

[[maybe_unused]] const char* get_error_string(int, char* buffer)
{
	return buffer;
}

} // namespace

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

int syserr()
{
	switch(errno) {
	case EEXIST:
		return Error::Exists;
	case EPERM:
	case EACCES:
		return Error::Denied;
	case ENOMEM:
		return Error::NoMem;
	case ENOENT:
		return Error::NotFound;
	case ENFILE:
	case EMFILE:
		return Error::OutOfFileDescs;
	case EFBIG:
		return Error::TooBig;
	case ENOSPC:
		return Error::NoSpace;
	case EROFS:
		return Error::ReadOnly;
	case EINVAL:
		return Error::BadParam;
	default:
		return Error::fromSystem(-errno);
	}
}

String getErrorString(int err)
{
	if(Error::isSystem(err)) {
		char buffer[256]{};
		auto r = strerror_r(-Error::toSystem(err), buffer, sizeof(buffer));
		return get_error_string(r, buffer);
	}

	return IFS::Error::toString(err);
}

} // namespace IFS::Host
