/*
 * ifstypes.h
 *
 *  Created on: 26 Aug 2018
 *      Author: mikee47
 *
 *  Platform-specific defininitions in here.
 */

#pragma once

#include <user_config.h>
#include <time.h>
#include <FakePgmSpace.h>
#include <BitManipulations.h>
#include <Services/Profiling/ElapseTimer.h>

#define snprintf(_buf, _length, _fmt, ...) m_snprintf(_buf, _length, _fmt, ##__VA_ARGS__)
