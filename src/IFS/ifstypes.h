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

#include "user_config.h"
#include <time.h>
#include "FakePgmSpace.h"
#include "WVector.h"

#ifdef __WIN32

#undef assert
#include <assert.h>
#include <stdio.h>

#else

#define snprintf(_buf, _length, _fmt, ...) m_snprintf(_buf, _length, _fmt, ##__VA_ARGS__)

#endif

#endif // __IFSTYPES_H
