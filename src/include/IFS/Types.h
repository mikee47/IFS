/*
 * Types.h
 *
 *  Created on: 26 Aug 2018
 *      Author: mikee47
 *
 *  Platform-specific defininitions in here.
 */

#pragma once

#include <assert.h>
#include <time.h>
#include <debug_progmem.h>
#include <BitManipulations.h>
#include <stringutil.h>
#include <sming_attr.h>
#include <WString.h>
#include <Data/BitSet.h>

#define snprintf(_buf, _length, _fmt, ...) m_snprintf(_buf, _length, _fmt, ##__VA_ARGS__)
