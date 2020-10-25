/*
 * StdFileMedia.h
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
class StdFileMedia : public Media
{
public:
	StdFileMedia(const char* filename, uint32_t size, uint32_t blockSize, Media::Attributes attr);
	~StdFileMedia() override;
	Media::Info getinfo() const override;
	int read(uint32_t offset, uint32_t size, void* buffer) override;
	int write(uint32_t offset, uint32_t size, const void* data) override;
	int erase(uint32_t offset, uint32_t size) override;

private:
	int m_file = -1;
	uint32_t m_blockSize = sizeof(uint32_t);
};

} // namespace IFS
