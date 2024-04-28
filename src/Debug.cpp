/****
 * Debug.cpp
 *
 * Copyright 2022 mikee47 <mike@sillyhouse.net>
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

#include "include/IFS/Debug.h"
#include <IFS/File.h>
#include <IFS/Directory.h>

namespace IFS::Debug
{
void printFsInfo(Print& out, FileSystem& fs)
{
	char namebuf[256];
	FileSystem::Info info(namebuf, sizeof(namebuf));
	int res = fs.getinfo(info);
	if(res < 0) {
		out << _F("fs.getinfo(): ") << fs.getErrorString(res) << endl;
	} else {
		out << info << endl;
	}
}

void printAttrInfo(Print& out, FileSystem& fs, const String& filename)
{
	File f(&fs);
	if(!f.open(filename)) {
		return;
	}
	auto callback = [&](AttributeEnum& e) -> bool {
		out << _F("  attr 0x") << String(unsigned(e.tag), HEX) << ' ' << e.tag << ' ' << e.attrsize << endl;
		m_printHex(_F("  ATTR"), e.buffer, e.size);
		return true;
	};
	char buffer[64];
	int res = f.enumAttributes(callback, buffer, sizeof(buffer));
	(void)res;
	debug_d("enumAttributes: %d", res);
}

int listDirectory(Print& out, FileSystem& fs, const String& path, Options options)
{
	out << _F("$ ls \"") << path << '\"' << endl;

	NameStat stat;
	int err = fs.stat(path, &stat);
	if(err < 0) {
		out << _F("stat('") << path << "'): " << fs.getErrorString(err) << endl;
		return err;
	}

	out.println(stat);
	if(options[Option::attributes]) {
		printAttrInfo(out, fs, path);
	}

	Directory dir(&fs);
	if(!dir.open(path)) {
		return dir.getLastError();
	}

	while(dir.next()) {
		out.println(dir.stat());
		String filename = path;
		filename += '/';
		filename += dir.stat().name.c_str();
		if(options[Option::attributes]) {
			printAttrInfo(out, fs, filename);
		}
	}

	if(options[Option::recurse]) {
		dir.rewind();
		while(dir.next()) {
			if(dir.stat().attr[FileAttribute::Directory]) {
				String subdir = path;
				if(subdir.length() != 0) {
					subdir += '/';
				}
				subdir += dir.stat().name;
				listDirectory(out, fs, subdir, options);
			}
		}
	}

	return dir.getLastError();
}

} // namespace IFS::Debug
