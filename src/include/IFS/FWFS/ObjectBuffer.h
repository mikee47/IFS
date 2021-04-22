/**
 * ObjectBuffer.h
 *
 * Copyright 2021 mikee47 <mike@sillyhouse.net>
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
 */

#pragma once

#include <Data/Stream/MemoryDataStream.h>
#include "../include/IFS/FWFS/Object.h"

namespace IFS
{
namespace FWFS
{
/**
 * @brief Class to manage writing object data into a stream
 */
class ObjectBuffer : public MemoryDataStream
{
public:
	void write(const void* data, size_t size)
	{
		auto n = MemoryDataStream::write(static_cast<const uint8_t*>(data), size);
		(void)n;
		assert(n == size);
	}

	void write(const String& value)
	{
		write(value.c_str(), value.length());
	}

	void write(const NameBuffer& value)
	{
		write(value.c_str(), value.length);
	}

	void write(uint32_t value)
	{
		return write(&value, sizeof(value));
	}

	void write(const Object& hdr, size_t extra, size_t bodySize)
	{
		size_t headerSize = hdr.contentOffset() + extra;
		if(bodySize != 0) {
			ensureCapacity(getSize() + headerSize + bodySize);
		}
		write(&hdr, headerSize);
	}

	void writeRef(Object::Type type, Object::ID objId)
	{
		Object hdr;
		hdr.setType(type, true);
		hdr.data8.ref.packedOffset = objId;
		size_t idSize = (objId <= 0xff) ? 1 : (objId <= 0xffff) ? 2 : (objId <= 0xffffff) ? 3 : 4;
		hdr.data8.setContentSize(idSize);
		write(hdr, idSize, 0);
	}

	Object::Type writeDataHeader(size_t size)
	{
		Object hdr;
		if(size <= 0xff) {
			hdr.setType(Object::Type::Data8);
			hdr.data8.setContentSize(size);
		} else if(size <= 0xffff) {
			hdr.setType(Object::Type::Data16);
			hdr.data16.setContentSize(size);
		} else {
			hdr.setType(Object::Type::Data24);
			hdr.data24.setContentSize(size);
		}
		write(hdr, 0, 0);
		return hdr.type();
	}

	void writeNamed(Object::Type type, const char* name, uint8_t namelen, TimeStamp mtime)
	{
		Object hdr;
		hdr.setType(type);
		hdr.data16.named.namelen = namelen;
		hdr.data16.named.mtime = mtime;
		write(hdr, hdr.data16.named.nameOffset(), namelen);
		write(name, namelen);
	}

	void fixupSize()
	{
		auto objptr = getStreamPointer();
		Object hdr;
		memcpy(&hdr, objptr, 4);
		hdr.setContentSize(available() - hdr.contentOffset());
		memcpy(const_cast<char*>(objptr), &hdr, 4);
	}
};

} // namespace FWFS
} // namespace IFS
