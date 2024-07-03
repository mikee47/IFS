/****
 * ArchiveStream.h
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
 ****/

#pragma once

#include <Data/Stream/DataSourceStream.h>

namespace IFS::FWFS
{
/**
 * @brief Virtual base class to support (file) data encryption and compression
 *
 * Encryption and compression are typically done in blocks of a fixed size.
 * To support these operations an instance of this class is created which encodes
 * data one block at a time. Each block is stored separately and the resulting file
 * consists of a chain of these blocks. This is natively supported by FWFS.
 *
 * If the final data size is known in advance then the implementation will return just
 * a single data stream.
 */
class IBlockEncoder
{
public:
	virtual ~IBlockEncoder() = default;

	/**
	 * @Implement this method and return nullptr when all blocks have been encoded.
	 *
	 * The stream returned must know it's size (i.e. available() must not return -1).
	 * The encoder owns any stream objects created so is responsible for destroying them
	 * when finished. This allows them to be re-used if appropriate.
	 */
	virtual IDataSourceStream* getNextStream() = 0;
};

class BasicEncoder : public IBlockEncoder
{
public:
	BasicEncoder(IDataSourceStream* stream) : stream(stream)
	{
	}

	IDataSourceStream* getNextStream() override
	{
		if(done) {
			stream.reset();
			return nullptr;
		}

		done = true;
		return stream.get();
	}

protected:
	std::unique_ptr<IDataSourceStream> stream;
	bool done{false};
};

} // namespace IFS::FWFS
