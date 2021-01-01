/*
 * Helpers.cpp
 *
 *  Created on: 27 Jan 2019
 *      Author: Mike
 */

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
