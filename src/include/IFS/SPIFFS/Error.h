/*
 * SpiffsError.h
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#pragma once

namespace IFS
{
namespace SPIFFS
{
int spiffsErrorToStr(int err, char* buffer, unsigned size);
}
} // namespace IFS
