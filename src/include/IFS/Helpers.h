/**
 * Helpers.h
 * Helper functions to assist with standard filesystem creation
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

#pragma once

#include "FileSystem.h"

namespace IFS
{
/**
 * @brief Create a SPIFFS filesystem
 * @param partition
 * @retval FileSystem* constructed filesystem object
 */
FileSystem* createSpiffsFilesystem(Storage::Partition partition);

/**
 * @brief Create a firmware filesystem
 * @param partition
 * @retval FileSystem* constructed filesystem object
 */
FileSystem* createFirmwareFilesystem(Storage::Partition partition);

/**
 * @brief Create a hybrid filesystem
 * @param fwfsPartition Base read-only filesystem partition
 * @param flashFileSystem The filesystem to use for writing
 * @retval FileSystem* constructed filesystem object
 */
FileSystem* createHybridFilesystem(Storage::Partition fwfsPartition, IFileSystem* flashFileSystem);

} // namespace IFS
