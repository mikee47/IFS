/*
 * FileMedia.h
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 *
 */

#pragma once

#include "Media.h"
#include "File/Stat.h"

namespace IFS
{
/**
 * @brief Implementation of IFS media layer on another IFS file
 */
class FileMedia : public Media
{
public:
	FileMedia(IFileSystem& fs, const char* filename, size_t size, Media::Attributes attr)
		: Media(size, attr), fileSys(fs)
	{
		open(filename);
	}

	FileMedia(IFileSystem& fs, const String& filename, size_t size, Media::Attributes attr)
		: FileMedia(fs, filename.c_str(), size, attr)
	{
	}

	FileMedia(IFileSystem& fs, File::Handle file, size_t size, Media::Attributes attr) : Media(size, attr), fileSys(fs)
	{
		attach(file);
	}

	~FileMedia();

	Media::Info getinfo() const override;
	int read(uint32_t offset, uint32_t size, void* buffer) override;
	int write(uint32_t offset, uint32_t size, const void* data) override;
	int erase(uint32_t offset, uint32_t size) override;

private:
	void open(const char* filename);
	void attach(File::Handle file);

	IFileSystem& fileSys;
	File::Handle m_file{-1};
};

} // namespace IFS
