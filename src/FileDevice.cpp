/*
 * FileDevice.cpp
 */

#include "include/Storage/FileDevice.h"

namespace Storage
{
bool FileDevice::read(uint32_t address, void* buffer, size_t len)
{
	int res = fileSystem.lseek(file, address, SeekOrigin::Start);
	if(size_t(res) != address) {
		return false;
	}

	res = fileSystem.read(file, buffer, len);
	return size_t(res) == len;
}

bool FileDevice::write(uint32_t address, const void* data, size_t len)
{
	int res = fileSystem.lseek(file, address, SeekOrigin::Start);
	if(size_t(res) != address) {
		return false;
	}

	res = fileSystem.write(file, data, len);
	return size_t(res) == len;
}

bool FileDevice::erase_range(uint32_t address, size_t len)
{
	constexpr size_t bufSize{512};
	uint8_t buffer[bufSize];
	memset(buffer, 0xff, sizeof(buffer));

	int res = fileSystem.lseek(file, address, SeekOrigin::Start);
	if(size_t(res) != address) {
		return false;
	}

	while(len > 0) {
		size_t toWrite = std::min(bufSize, len);
		int res = fileSystem.write(file, buffer, toWrite);
		if(size_t(res) != toWrite) {
			return false;
		}
		len -= toWrite;
	}

	return true;
}

} // namespace Storage
