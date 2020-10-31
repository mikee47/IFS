/*
 * FileMedia.h
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 *
 * Implementation of IFS media layer on standard C FILE stream.
 *
 */

#pragma once

#include <IFS/Media.h>

namespace IFS
{
namespace Host
{
class FileMedia : public Media
{
public:
	FileMedia(const String& filename, uint32_t size, uint32_t blockSize, Media::Attributes attr);
	~FileMedia();
	Media::Info getinfo() const override;
	int read(uint32_t offset, uint32_t size, void* buffer) override;
	int write(uint32_t offset, uint32_t size, const void* data) override;
	int erase(uint32_t offset, uint32_t size) override;

private:
	int m_file{-1};
	uint32_t m_blockSize{sizeof(uint32_t)};
};

} // namespace Host
} // namespace IFS
