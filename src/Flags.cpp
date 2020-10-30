/*
 * Flags.cpp
 *
 *  Created on: 31 Aug 2018
 *      Author: mikee47
 */

#include "Flags.h"

namespace IFS
{
String flagsToStr(uint32_t flags, const FSTR::Vector<FlashString>& strings)
{
	String s;
	unsigned f = 0;
	for(auto& fs : strings) {
		if(!bitRead(flags, f++)) {
			continue;
		}

		if(s) {
			s += ',';
			s += ' ';
		}

		s += fs;
	}

	return s;
}

} // namespace IFS
