/*
 * FileDevice.h
 */

#include <Storage/CustomDevice.h>
#include "../IFS/FileSystem.h"

namespace Storage
{
/**
 * @brief Read-only partition on a stream object
 * @note Writes not possible as streams always append data, cannot do random writes
 */
class FileDevice : public CustomDevice
{
public:
	FileDevice(const String& name, IFS::IFileSystem& fileSys, IFS::File::Handle file, size_t size)
		: name(name), size(size), fileSystem(fileSys), file(file)
	{
	}

	FileDevice(const String& name, IFS::IFileSystem& fileSys, IFS::File::Handle file)
		: name(name), size(fileSys.getSize(file)), fileSystem(fileSys), file(file)
	{
	}

	~FileDevice()
	{
		fileSystem.close(file);
	}

	String getName() const override
	{
		return name.c_str();
	}

	Type getType() const override
	{
		return Type::file;
	}

	size_t getSize() const override
	{
		return size;
	}

	size_t getBlockSize() const override
	{
		return sizeof(uint32_t);
	}

	bool read(uint32_t address, void* buffer, size_t len) override;
	bool write(uint32_t address, const void* data, size_t len) override;
	bool erase_range(uint32_t address, size_t len) override;

private:
	CString name;
	size_t size;
	IFS::IFileSystem& fileSystem;
	IFS::File::Handle file;
};

} // namespace Storage
