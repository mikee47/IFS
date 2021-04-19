/**
 * ObjectStore.h
 *
 * Created on: 1 Sep 2018
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

#include "Object.h"
#include "Error.h"
#include <Storage/Partition.h>

namespace IFS
{
/**
 * @brief gives the identity and location of an FWFS object
 */
struct ObjRef {
	uint32_t offset{0}; ///< Offset from start of image
	Object::ID id{0};
	uint8_t readCount{0}; ///< For profiling

	ObjRef(Object::ID objID = 0) : id(objID)
	{
	}
};

/**
 * @brief FWFS Object Descriptor
 */
struct FWObjDesc {
	Object obj; ///< The object structure
	ObjRef ref; ///< location

	FWObjDesc()
	{
	}

	FWObjDesc(const ObjRef& objRef) : ref(objRef)
	{
	}

	FWObjDesc(uint32_t objId) : ref(objId)
	{
	}

	uint32_t nextOffset() const
	{
		return ref.offset + obj.sizeAligned();
	}

	// Move to next object location
	void next()
	{
		ref.offset = nextOffset();
		++ref.id;
	}

	uint32_t contentOffset() const
	{
		return ref.offset + obj.contentOffset();
	}

	uint32_t childTableOffset() const
	{
		return ref.offset + obj.childTableOffset();
	}
};

} // namespace IFS
