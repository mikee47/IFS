/*
 * SeekOrigin.h
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#pragma once

namespace IFS
{
namespace File
{
/**
 * @brief File seek origins
 * @note these values are fixed in stone so will never change. They only need to
 * be remapped if a filing system uses different values.
 */
enum class SeekOrigin {
	Start = 0,   ///< Start of file
	Current = 1, ///< Current position in file
	End = 2		 ///< End of file
};

} // namespace File
} // namespace IFS
