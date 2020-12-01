/*
 * Util.h
 *
 *  Created on: 1 December 2020
 *      Author: mikee47
 *
 * Internal shared utility stuff for dealing with Host filing API
 *
 */

#pragma once

#include "../Error.h"
#include "../File/OpenFlags.h"
#include <errno.h>
#include <WString.h>

struct stat;

namespace IFS
{
namespace Host
{
/**
 * @brief Get IFS error code for the current system errno
 */
inline int syserr()
{
	return Error::fromSystem(-errno);
}

/**
 * @brief Get corresponding host flags for given IFS flags
 */
int mapFlags(File::OpenFlags flags);

String getErrorString(int err);

} // namespace Host
} // namespace IFS
