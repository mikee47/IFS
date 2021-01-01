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
/**
 * @brief Create a firmware filesystem
 * @param partition
 * @retval IFileSystem* constructed filesystem object
 */
IFileSystem* createFirmwareFilesystem(Storage::Partition partition);

/**
 * @brief Create a hybrid filesystem
 * @param fwfsPartition
 * @param spiffsPartition
 * @retval IFileSystem* constructed filesystem object
 */
IFileSystem* createHybridFilesystem(Storage::Partition fwfsPartition, Storage::Partition spiffsPartition);

} // namespace IFS
