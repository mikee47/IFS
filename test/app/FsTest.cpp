#include <FsTest.h>

String getErrorString(FileSystem* fs, int err)
{
	return fs ? fs->getErrorString(err) : IFS::Error::toString(err);
}

String toString(Flag f)
{
	switch(f) {
#define XX(name, desc)                                                                                                 \
	case Flag::name:                                                                                                   \
		return F(#name);
		FLAG_MAP(XX)
#undef XX
	default:
		return nullptr;
	}
}
