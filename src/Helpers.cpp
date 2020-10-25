/*
 * Helpers.cpp
 *
 *  Created on: 27 Jan 2019
 *      Author: Mike
 */

#include "include/IFS/Helpers.h"
#include "include/IFS/FlashMedia.h"
#include "include/IFS/FWObjectStore.h"
#include "include/IFS/HybridFileSystem.h"
#include <spiffs_sming.h>

#include <SystemClock.h>

#ifdef ARCH_HOST
#include <IFS/MemoryMedia.h>
using MediaType = IFS::MemoryMedia;
#else
using MediaType = IFS::FlashMedia;
#endif

namespace IFS
{
/** @brief required by IFS, platform-specific */
time_t fsGetTimeUTC()
{
	return SystemClock.now(eTZ_UTC);
}

IFileSystem* CreateFirmwareFilesystem(const void* fwfsImageData)
{
	auto fwMedia = new MediaType(fwfsImageData, eFMA_ReadOnly);
	if(fwMedia == nullptr) {
		return nullptr;
	}

	auto store = new FWObjectStore(fwMedia);
	if(store == nullptr) {
		delete fwMedia;
		return nullptr;
	}

	auto fs = new FirmwareFileSystem(store);
	if(fs == nullptr) {
		delete store;
	}

	return fs;
}

IFileSystem* CreateHybridFilesystem(const void* fwfsImageData)
{
	auto fwMedia = new MediaType(fwfsImageData, eFMA_ReadOnly);
	if(fwMedia == nullptr) {
		return nullptr;
	}

	auto store = new FWObjectStore(fwMedia);
	if(store == nullptr) {
		delete fwMedia;
		return nullptr;
	}

	auto cfg = spiffs_get_storage_config();
	auto ffsMedia = new FlashMedia(cfg.phys_addr, cfg.phys_size, eFMA_ReadWrite);

	auto fs = new HybridFileSystem(store, ffsMedia);

	if(fs == nullptr) {
		delete ffsMedia;
		delete store;
	}

	return fs;
}

} // namespace IFS
