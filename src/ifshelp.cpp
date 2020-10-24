/*
 * ifshelp.cpp
 *
 *  Created on: 27 Jan 2019
 *      Author: Mike
 */

#include "IFS/ifshelp.h"
#include "IFS/IFSFlashMedia.h"
#include "IFS/FWObjectStore.h"
#include "IFS/HybridFileSystem.h"
#include "spiffs_sming.h"

#include "SystemClock.h"

#ifdef ARCH_HOST
#include "IFS/IFSMemoryMedia.h"
typedef IFSMemoryMedia FlashMedia;
#else
typedef IFSFlashMedia FlashMedia;
#endif

/** @brief required by IFS, platform-specific */
time_t fsGetTimeUTC()
{
	return SystemClock.now(eTZ_UTC);
}

IFileSystem* CreateFirmwareFilesystem(const void* fwfsImageData)
{
	auto fwMedia = new FlashMedia(fwfsImageData, eFMA_ReadOnly);
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
	auto fwMedia = new FlashMedia(fwfsImageData, eFMA_ReadOnly);
	if(fwMedia == nullptr) {
		return nullptr;
	}

	auto store = new FWObjectStore(fwMedia);
	if(store == nullptr) {
		delete fwMedia;
		return nullptr;
	}

	auto cfg = spiffs_get_storage_config();
	auto ffsMedia = new IFSFlashMedia(cfg.phys_addr, cfg.phys_size, eFMA_ReadWrite);

	auto fs = new HybridFileSystem(store, ffsMedia);

	if(fs == nullptr) {
		delete ffsMedia;
		delete store;
	}

	return fs;
}
