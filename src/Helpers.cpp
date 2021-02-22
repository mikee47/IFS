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
#include "include/IFS/HYFS/FileSystem.h"
#include <spiffs_sming.h>
#include <Storage.h>
#include <SystemClock.h>

namespace IFS
{
/** @brief required by IFS, platform-specific */
time_t fsGetTimeUTC()
{
	return SystemClock.now(eTZ_UTC);
}

IFileSystem* createFirmwareFilesystem(Storage::Partition partition)
{
	auto store = new FWFS::ObjectStore(partition);
	if(store == nullptr) {
		return nullptr;
	}

	auto fs = new FWFS::FileSystem(store);
	if(fs == nullptr) {
		delete store;
	}

	return fs;
}

IFileSystem* createHybridFilesystem(Storage::Partition fwfsPartition, Storage::Partition spiffsPartition)
{
	auto store = new FWFS::ObjectStore(fwfsPartition);
	auto fs = new HYFS::FileSystem(store, spiffsPartition);

	if(fs == nullptr) {
		delete store;
	}

	return fs;
}

} // namespace IFS
