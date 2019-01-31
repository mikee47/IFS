/*
 * ifstypes.h
 *
 *  Created on: 26 Aug 2018
 *      Author: mikee47
 *
 *  Platform-specific defininitions in here.
 */

#ifndef __IFSTYPES_H
#define __IFSTYPES_H

#if defined(SMING_INCLUDED)

#include "user_config.h"
#include <time.h>
#include "FakePgmSpace.h"
#include "../Services/Profiling/ElapseTimer.h"

#define snprintf(_buf, _length, _fmt, ...) m_snprintf(_buf, _length, _fmt, ##__VA_ARGS__)

#elif defined(__WIN32)

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <algorithm>
#include "ElapseTimer.h"

// Align a size up to the nearest word boundary
#define ALIGNUP(_n) (((_n) + 3) & ~3)

#define __packed __attribute__((packed))

#define debug_none(fmt, ...)                                                                                           \
	do {                                                                                                               \
	} while(0)

#define debugf(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define debug_e(fmt, ...) debugf("ERROR: " fmt, ##__VA_ARGS__)
#define debug_w(fmt, ...) debugf("WARNING: " fmt, ##__VA_ARGS__)
#define debug_i debugf
#define debug_d debug_none

#define _BV(bit) (1 << (unsigned)(bit))
#define bitSet(value, bit) ((value) |= _BV(bit))
#define bitClear(value, bit) ((value) &= ~_BV(bit))
#define bitRead(value, bit) (((value) >> unsigned(bit)) & 0x01)

#define PROGMEM
#define DEFINE_PSTR(_name, _str) const char _name[] PROGMEM = _str;
typedef const char* PGM_P;

#define _F(str) str

#define LOAD_PSTR(_name, _flash_str)                                                                                   \
	char _name[ALIGNUP(sizeof(_flash_str))] __attribute__((aligned(4)));                                               \
	memcpy(_name, _flash_str, sizeof(_name));

#define memcpy_P(dest, src, num) memcpy((dest), (src), (num))
#define strlen_P(a) strlen((a))
#define strncpy_P(dest, src, size) strncpy((dest), (src), (size))
#define strcasecmp_P(a, b) strcasecmp((a), (b))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

/** @brief required by IFS, platform-specific */
inline time_t fsGetTimeUTC()
{
	return time(nullptr);
}

#else

#error "ifstypes.h requires updating for this platform"

#endif

#endif // __IFSTYPES_H
