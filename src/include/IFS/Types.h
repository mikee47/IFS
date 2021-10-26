/****
 * Types.h
 * Platform-specific defininitions
 *
 * Created on: 26 Aug 2018
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

#include <assert.h>
#include <time.h>
#include <debug_progmem.h>
#include <BitManipulations.h>
#include <stringutil.h>
#include <sming_attr.h>
#include <WString.h>
#include <Data/BitSet.h>

#define snprintf(_buf, _length, _fmt, ...) m_snprintf(_buf, _length, _fmt, ##__VA_ARGS__)
