/**
 * File.cpp
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

#include "include/IFS/File.h"

namespace IFS
{
constexpr OpenFlags File::ReadOnly;
constexpr OpenFlags File::WriteOnly;
constexpr OpenFlags File::ReadWrite;
constexpr OpenFlags File::Create;
constexpr OpenFlags File::Append;
constexpr OpenFlags File::Truncate;
constexpr OpenFlags File::CreateNewAlways;

} // namespace IFS
