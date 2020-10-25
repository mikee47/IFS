/*
 * Util.h
 *
 *  Created on: 10 Sep 2018
 *      Author: Mike
 *
 *  Various bits for file system implementations to use
 */

#pragma once

#include "Error.h"

namespace IFS
{
/*
 * Paths equal to "/" or "" are empty and considered equivalent to nullptr.
 * Methods or functions can use this macro to resolve these for simpler parsing.
 */
#define FS_CHECK_PATH(path)                                                                                            \
	if(path) {                                                                                                         \
		if(*path == '/') {                                                                                             \
			++path;                                                                                                    \
		}                                                                                                              \
		if(*path == '\0') {                                                                                            \
			path = nullptr;                                                                                            \
		}                                                                                                              \
	}

} // namespace IFS
