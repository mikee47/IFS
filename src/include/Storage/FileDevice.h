/****
 * FileDevice.h
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

#pragma once

#include <Storage/Device.h>
#include "../IFS/FileSystem.h"

namespace Storage
{
/**
 * @brief Create custom storage device using backing file
 */
class FileDevice : public Device
{
public:
	/**
	 * @brief Construct a file device with custom size
	 * @param name Name of device
	 * @param fileSys File system where file is located
	 * @param file Handle to open file
	 * @param size Size of device in bytes
	 */
	FileDevice(const String& name, IFS::IFileSystem& fileSys, IFS::FileHandle file, storage_size_t size)
		: name(name), size(size), fileSystem(fileSys), file(file)
	{
	}

	/**
	 * @brief Construct a device using existing file
	 * @param name Name of device
	 * @param fileSys File system where file is located
	 * @param file Handle to open file
	 *
	 * Device will match size of existing file
	 */
	FileDevice(const String& name, IFS::IFileSystem& fileSys, IFS::FileHandle file)
		: name(name), fileSystem(fileSys), file(file)
	{
		size = IFS::FileSystem::cast(fileSys).getSize(file);
		auto blockSize = getBlockSize();
		auto blockCount = (size + blockSize - 1) / blockSize;
		size = blockCount * blockSize;
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

	storage_size_t getSize() const override
	{
		return size;
	}

	size_t getBlockSize() const override
	{
		// Use block size compatible with most disk drives
		return 512;
	}

	bool read(storage_size_t address, void* buffer, size_t len) override;
	bool write(storage_size_t address, const void* data, size_t len) override;
	bool erase_range(storage_size_t address, storage_size_t len) override;

private:
	CString name;
	storage_size_t size;
	IFS::IFileSystem& fileSystem;
	IFS::FileHandle file;
};

} // namespace Storage
