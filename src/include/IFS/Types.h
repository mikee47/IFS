/****
 * Types.h
 * Platform-specific definitions
 *
 * Created on: 26 Aug 2018
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

#include <assert.h>
#include <time.h>
#include <debug_progmem.h>
#include <BitManipulations.h>
#include <stringutil.h>
#include <sming_attr.h>
#include <WString.h>
#include <Data/BitSet.h>
#include <Storage/Types.h>

using volume_size_t = storage_size_t;

#ifdef ENABLE_FILE_SIZE64

#ifndef ENABLE_STORAGE_SIZE64
static_assert(false, "ENABLE_FILE_SIZE64 requires ENABLE_STORAGE_SIZE64 also");
#endif

using file_size_t = uint64_t;
using file_offset_t = int64_t;

#else

using file_size_t = uint32_t;
using file_offset_t = int32_t;

#endif
