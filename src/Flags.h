/*
 * Flags.h
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#pragma once

#include "include/IFS/Types.h"
#include <FlashString/String.hpp>
#include <FlashString/Vector.hpp>

namespace IFS
{
/**
 * @brief convert a set of flags into a comma-separated list
 * @param flags
 * @param strings 0-based vector of strings corresponding to bit values
 * @retval String
 */
String flagsToStr(uint32_t flags, const FSTR::Vector<FSTR::String>& strings);

} // namespace IFS
