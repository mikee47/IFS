/*
 * FileMedia.cpp
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 */

#include "include/IFS/FileSystem.h"
#include "include/IFS/FileMedia.h"
#include "include/IFS/Error.h"

#define CHECK_FILE()                                                                                                   \
	if(m_file < 0) {                                                                                                   \
		return Error::NoMedia;                                                                                         \
	}

#define SEEK(_offset)                                                                                                  \
	{                                                                                                                  \
		auto off = _offset;                                                                                            \
		if(fileSys.lseek(m_file, off, SeekOrigin::Start) != (int)off) {                                          \
			return Error::BadExtent;                                                                                   \
		}                                                                                                              \
	}

namespace IFS
{
FileMedia::~FileMedia()
{
	if(m_file >= 0) {
		fileSys.close(m_file);
	}
}

void FileMedia::open(const char* filename)
{
	File::OpenFlags flags = File::OpenFlag::Read;
	if(!attr()[Attribute::ReadOnly]) {
		flags |= File::OpenFlag::Create | File::OpenFlag::Write;
	}
	auto file = fileSys.open(filename, flags);
	if(file < 0) {
		debug_e("FileMedia failed to open '%s'", filename);
	} else {
		debug_i("FileMedia opened '%s' as #%d", filename, file);
		attach(file);
	}
}

void FileMedia::attach(File::Handle file)
{
	int len = fileSys.lseek(file, 0, SeekOrigin::End);
	if(len < 0) {
		debug_e("FileMedia #%d seek error %s", file, fileSys.getErrorString(len).c_str());
		fileSys.close(file);
		return;
	}

	/*
	 * If file is larger than indicated size, use that as our initial media size,
	 * alternatively if we're using the media in read/write then ensure the backing
	 * file is at least as large as the maximum size indicated.
	 */
	if(len > int(m_size)) {
		m_size = len;
	} else if(int(m_size) > len && !attr()[Attribute::ReadOnly]) {
		if(fileSys.truncate(file, m_size) < 0) {
			debug_e("FileMedia failed to truncate #%d to %u bytes", file, m_size);
			fileSys.close(file);
			return;
		}
	}

	debug_i("Opened FileMedia #%d, %u bytes", file, m_size);
	m_file = file;
}

Media::Info FileMedia::getinfo() const
{
	Media::Info info{
		.type = Type::File,
		.bus = Bus::System,
		.blockSize = 1,
	};

	return info;
}

int FileMedia::read(uint32_t offset, uint32_t size, void* buffer)
{
	CHECK_FILE();
	FS_CHECK_EXTENT(offset, size);
	SEEK(offset);
	int n = fileSys.read(m_file, buffer, size);
	return (n == int(size)) ? FS_OK : Error::ReadFailure;
}

int FileMedia::write(uint32_t offset, uint32_t size, const void* data)
{
	CHECK_FILE();
	FS_CHECK_EXTENT(offset, size);
	FS_CHECK_WRITEABLE();
	SEEK(offset);
	int n = fileSys.write(m_file, data, size);
	return (n == int(size)) ? FS_OK : Error::WriteFailure;
}

int FileMedia::erase(uint32_t offset, uint32_t size)
{
	debug_i("FileMedia::erase(0x%08X, 0x%08X)", offset, size);

	uint8_t tmp[size];
	memset(tmp, 0xFF, size);
	return write(offset, size, tmp);
}

} // namespace IFS
