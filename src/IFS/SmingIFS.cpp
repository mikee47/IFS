/*
 * SmingIFS.cpp
 *
 *  Created on: 26 Aug 2018
 *      Author: mikee47
 *
 *  Sming-specific code
 */

#include "SystemClock.h"

/** @brief required by IFS, platform-specific */
time_t fsGetTimeUTC()
{
	return SystemClock.now(eTZ_UTC);
}
