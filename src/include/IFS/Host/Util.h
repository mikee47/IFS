/**
 * Util.h
 * Internal shared utility stuff for dealing with Host filing API
 *
 * Created on: 1 December 2020
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

#include "../Error.h"
#include "../OpenFlags.h"
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
int mapFlags(OpenFlags flags);

String getErrorString(int err);

} // namespace Host
} // namespace IFS
