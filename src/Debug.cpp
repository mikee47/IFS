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

namespace IFS
{
namespace Debug
{
namespace
{
// Displays as local time
String timeToStr(time_t t, const char* dtsep)
{
	struct tm* tm = localtime(&t);
	if(tm == nullptr) {
		return nullptr;
	}
	char buffer[64];
	m_snprintf(buffer, sizeof(buffer), _F("%02u/%02u/%04u%s%02u:%02u:%02u"), tm->tm_mday, tm->tm_mon + 1,
			   1900 + tm->tm_year, dtsep, tm->tm_hour, tm->tm_min, tm->tm_sec);
	return buffer;
}

} // namespace

void printFsInfo(Print& out, FileSystem& fs)
{
	char namebuf[256];
	FileSystem::Info info(namebuf, sizeof(namebuf));
	int res = fs.getinfo(info);
	if(res < 0) {
		out.print("fileSystemGetInfo(): ");
		out.println(fs.getErrorString(res));
		return;
	}

	auto println = [&](const char* tag, String value) {
		String s(tag);
		s.reserve(16);
		s += ':';
		while(s.length() < 16) {
			s += ' ';
		}
		out.print(s);
		out.println(value);
	};

	println(_F("type"), toString(info.type));
	println(_F("partition"), info.partition.name());
	if(info.partition) {
		println(_F(" type"), info.partition.longTypeString());
		println(_F(" device"), info.partition.getDeviceName());
	}
	println(_F("maxNameLength"), String(info.maxNameLength));
	println(_F("maxPathLength"), String(info.maxPathLength));
	println(_F("attr"), toString(info.attr));
	println(_F("volumeID"), String(info.volumeID, HEX));
	println(_F("name"), info.name.c_str());
	println(_F("volumeSize"), String(info.volumeSize));
	println(_F("freeSpace"), String(info.freeSpace));
}

void printFileInfo(Print& out, const Stat& stat)
{
	FileSystem::Info info;
	stat.fs->getinfo(info);
	out.printf(_F("%-50s %8u %s #0x%08x %s %s [%s] {%s, %u}\r\n"), stat.name.c_str(), stat.size,
			   toString(info.type).c_str(), stat.id, timeToStr(stat.mtime, " ").c_str(), toString(stat.acl).c_str(),
			   toString(stat.attr).c_str(), toString(stat.compression.type).c_str(), stat.compression.originalSize);
}

void printAttrInfo(Print& out, FileSystem& fs, const String& filename)
{
	File f(&fs);
	if(!f.open(filename)) {
		return;
	}
	auto callback = [&](AttributeEnum& e) -> bool {
		out.printf(_F("  attr 0x%04x %s, %u bytes\r\n"), unsigned(e.tag), toString(e.tag).c_str(), e.attrsize);
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
	out.print(_F("$ ls "));
	out.println(path);

	NameStat stat;
	int err = fs.stat(path, &stat);
	if(err < 0) {
		out.printf("stat('%s'): %s\r\n", path.c_str(), fs.getErrorString(err).c_str());
		return err;
	}

	printFileInfo(out, stat);
	if(options[Option::attributes]) {
		printAttrInfo(out, fs, path);
	}

	Directory dir(&fs);
	if(!dir.open(path)) {
		return dir.getLastError();
	}

	while(dir.next()) {
		printFileInfo(out, dir.stat());
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

} // namespace Debug
} // namespace IFS
