/*
 * HostUtil.cpp
 *
 */

#undef __STRICT_ANSI__
#undef _GNU_SOURCE

#include "include/IFS/Host/Util.h"
#include <fcntl.h>

#include <string.h>
extern "C" int strerror_r(int, char*, size_t);

#ifndef O_BINARY
#define O_BINARY 0
#endif

namespace IFS
{
namespace Host
{
int mapFlags(File::OpenFlags flags)
{
	int ret = O_BINARY;
	if(flags[File::OpenFlag::Append]) {
		ret |= O_APPEND;
	}
	if(flags[File::OpenFlag::Create]) {
		ret |= O_CREAT;
	}
	if(flags[File::OpenFlag::Truncate]) {
		ret |= O_TRUNC;
	}
	if(flags[File::OpenFlag::Read]) {
		if(flags[File::OpenFlag::Write]) {
			ret |= O_RDWR;
		} else {
			ret |= O_RDONLY;
		}
	} else if(flags[File::OpenFlag::Write]) {
		ret |= O_WRONLY;
	}
	return ret;
}

String getErrorString(int err)
{
	if(Error::isSystem(err)) {
		char buffer[256];
		buffer[0] = '\0';
		auto r = strerror_r(-Error::toSystem(err), buffer, sizeof(buffer));
		(void)r;
		return String(buffer);
	}

	return IFS::Error::toString(err);
}

} // namespace Host
} // namespace IFS
