/*
 * StdFileMedia.h
 *
 *  Created on: 18 Aug 2018
 *      Author: mikee47
 *
 * Implementation of IFS media layer on standard C FILE stream.
 *
 */

#ifndef __STDFILE_MEDIA_H
#define __STDFILE_MEDIA_H

#include "IFS/IFSMedia.h"

class StdFileMedia : public IFSMedia
{
public:
	StdFileMedia(const char* filename, uint32_t size, uint32_t blockSize, FSMediaAttributes attr);
	virtual ~StdFileMedia();
	virtual FSMediaInfo getinfo() const;
	virtual int read(uint32_t offset, uint32_t size, void* buffer);
	virtual int write(uint32_t offset, uint32_t size, const void* data);
	virtual int erase(uint32_t offset, uint32_t size);

private:
	int _file = -1;
	uint32_t _blockSize = sizeof(uint32_t);
};

#endif // __STDFILE_MEDIA_H
