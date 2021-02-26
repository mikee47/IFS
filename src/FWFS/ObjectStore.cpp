/**
 * ObjectStore.cpp
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

#include "../include/IFS/FWFS/ObjectStore.h"

// First object located immediately after start marker in image
#define FWFS_BASE_OFFSET sizeof(uint32_t)

namespace IFS
{
namespace FWFS
{
int ObjectStore::initialise()
{
	if(!partition) {
		return Error::NoMedia;
	}

	uint32_t marker;
	if(!partition.read(0, marker)) {
		return Error::ReadFailure;
	}

	if(marker != FWFILESYS_START_MARKER) {
		debug_e("Filesys start marker invalid: found 0x%08x, expected 0x%08x", marker, FWFILESYS_START_MARKER);
		return Error::BadFileSystem;
	}

	return FS_OK;
}

int ObjectStore::mounted(const FWObjDesc& od)
{
	// Check the end marker
	uint32_t marker;
	auto offset = FWFS_BASE_OFFSET + od.nextOffset();
	if(partition.read(offset, marker)) {
		if(marker != FWFILESYS_END_MARKER) {
			debug_e("Filesys end marker invalid: found 0x%08x, expected 0x%08x", marker, FWFILESYS_END_MARKER);
			return Error::BadFileSystem;
		}
	}

#ifdef FWFS_OBJECT_CACHE
	cache.initialise(od.obj.id + 1);
#endif

	flags[Flag::mounted] = true;
	return FS_OK;
}

int ObjectStore::open(FWObjDesc& od)
{
	// The object we're looking for
	Object::ID objId = od.ref.id;

	// Start search with first child
	od.ref.offset = 0;
	od.ref.id = 1; // We don't use id #0

	if(objId > lastFound.id && lastFound.id > od.ref.id) {
		od.ref = lastFound;
	}

	int res;
	while((res = readHeader(od)) >= 0 && od.ref.id != objId) {
		od.next();
	}

	if(res >= 0) {
		lastFound = od.ref;
	} else if(res == Error::NoMoreFiles) {
		res = Error::NotFound;
	}

	return res;
}

int ObjectStore::openChild(const FWObjDesc& parent, const FWObjDesc& child, FWObjDesc& od)
{
	if(!child.obj.isRef()) {
		od = child;
		od.ref.offset += parent.childTableOffset();
		return FS_OK;
	}

	od.ref = ObjRef(child.ref, child.obj.data8.ref.id);

	int res = open(od);
	if(res >= 0) {
		if(od.obj.type() != child.obj.type()) {
			// Reference must point to object of same type
			res = Error::BadObject;
		} else if(od.obj.isRef()) {
			// Reference must point to an actual object, not another reference
			res = Error::BadObject;
		}
	}

	return res;
}

int ObjectStore::close(FWObjDesc& od)
{
	// Nothing to do for FWRO
	return FS_OK;
}

int ObjectStore::readHeader(FWObjDesc& od)
{
	//	debug_d("readObject(0x%08X), offset = 0x%08X, sod = %u", &od, od.offset, sizeof(od.obj));
	assert(sizeof(od.obj) == 8);
	++od.ref.readCount;
	// First object ID is 1
	if(od.ref.offset == 0) {
		od.ref.id = 1;
	}
	int res = partition.read(FWFS_BASE_OFFSET + od.ref.offset, od.obj) ? FS_OK : Error::ReadFailure;

	/*
	During volume mount, readHeader() is called repeatedly to iterate base objects.
	We had this check for consecutive IDs but it doesn't apply to SPIFFS which allocates
	its own IDs. Also, readHeader might be used on child objects in which case the ID
	isn't applicable. So just leave this code here until I decide what to do with it.

	ID lastObjectID{0};
	...
	// This check is only applicable to FWRO
	if(od.ref.id < lastObjectID) {
		debug_e("FWFS object ID mismatch at 0x%08X: found %u, last ID was %u", od.ref.offset, od.ref.id, lastObjectID);
		res = Error::BadFileSystem;
		break;
	}
	lastObjectID = od.ref.id;
*/

#ifdef FWFS_OBJECT_CACHE
	if(res >= 0) {
		if(isMounted()) {
			cache.improve(od.ref, objIndex);
		} else {
			cache.add(od.ref);
		}
	}
#endif

	return res;
}

int ObjectStore::readChildHeader(const FWObjDesc& parent, FWObjDesc& child)
{
	if(child.ref.offset >= parent.obj.childTableSize()) {
		return Error::EndOfObjects;
	}

	// Get the absolute offset for the child object
	uint32_t tableOffset = parent.childTableOffset();
	child.ref.offset += tableOffset;
	int res = readHeader(child);
	child.ref.offset -= tableOffset;
	return res;
}

int ObjectStore::readContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer)
{
	offset += FWFS_BASE_OFFSET + od.contentOffset();
	return partition.read(offset, buffer, size) ? FS_OK : Error::ReadFailure;
}

} // namespace FWFS
} // namespace IFS
