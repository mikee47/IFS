/*
 * Helpers.h
 *
 *  Created on: 27 Jan 2019
 *      Author: Mike
 *
 * Helper functions to assist with standard filesystem creation
 */

#pragma once

#include "IFS/FileSystem.h"

namespace IFS
{
/** @brief Create a firmware filesystem
 *  @param fwfsImageData
 *  @retval IFileSystem* constructed filesystem object
 */
IFileSystem* CreateFirmwareFilesystem(const void* fwfsImageData);

/** @brief Create a hybrid filesystem
 *  @param fwfsImageData
 *  @retval IFileSystem* constructed filesystem object
 *  @note SPIFFS configuration is obtained via spiffs_get_storage_config()
 */
IFileSystem* CreateHybridFilesystem(const void* fwfsImageData);

} // namespace IFS
