/*
 * SpiffsError.h
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#pragma once

#include "../Types.h"

namespace IFS
{
namespace SPIFFS
{
inline bool isSpiffsError(int err)
{
	return err <= -10000;
}

String spiffsErrorToStr(int err);
} // namespace SPIFFS
} // namespace IFS
