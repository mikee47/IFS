/**
 * Helpers.cpp
 *
 * Created on: 27 Jan 2019
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

#include "include/IFS/Helpers.h"
#include "include/IFS/FWFS/ObjectStore.h"
#include "include/IFS/FWFS/FileSystem.h"
#include "include/IFS/HYFS/FileSystem.h"
#include <Storage.h>
#include <SystemClock.h>

namespace IFS
{
/** @brief required by IFS, platform-specific */
time_t fsGetTimeUTC()
{
	return SystemClock.now(eTZ_UTC);
}

FileSystem* createFirmwareFilesystem(Storage::Partition partition)
{
	auto store = new FWFS::ObjectStore(partition);
	if(store == nullptr) {
		return nullptr;
	}

	auto fs = new FWFS::FileSystem(store);
	if(fs == nullptr) {
		delete store;
	}

	return FileSystem::cast(fs);
}

FileSystem* createHybridFilesystem(Storage::Partition fwfsPartition, IFileSystem* flashFileSystem)
{
	if(flashFileSystem == nullptr) {
		return nullptr;
	}

	auto fwfs = createFirmwareFilesystem(fwfsPartition);
	if(fwfs == nullptr) {
		delete flashFileSystem;
		return nullptr;
	}

	auto fs = new HYFS::FileSystem(fwfs, flashFileSystem);
	return FileSystem::cast(fs);
}

} // namespace IFS
