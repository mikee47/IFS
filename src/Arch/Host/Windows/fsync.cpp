#include "fsync.h"
#include <io.h>
#include <errno.h>
#include <WinBase.h>

namespace
{
HANDLE getHandle(int file)
{
	return HANDLE(_get_osfhandle(file));
}

} // namespace

int fsync(int file)
{
	auto errorCode = FlushFileBuffers(getHandle(file));
	if(errorCode == 0) {
		errno = 0;
		return 0;
	}
	errno = EBADF;
	return -1;
}
