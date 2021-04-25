/**
 * Directory.cpp
 *
 * Created: May 2019
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

#include "include/IFS/Directory.h"

namespace IFS
{
bool Directory::open(const String& dirName)
{
	GET_FS(false)

	DirHandle dir;
	int err = fs->opendir(dirName, dir);
	if(!check(err)) {
		debug_w("Directory '%s' open error: %s", dirName.c_str(), fs->getErrorString(err).c_str());
		return false;
	}

	close();
	name = dirName ?: "";
	this->dir = dir;

	return true;
}

void Directory::close()
{
	if(dir != nullptr) {
		auto fs = getFileSystem();
		assert(fs != nullptr);
		fs->closedir(dir);
		dir = nullptr;
	}
	lastError = FS_OK;
}

bool Directory::rewind()
{
	GET_FS(false)

	int err = fs->rewinddir(dir);
	return err == FS_OK;
}

String Directory::getPath() const
{
	String path('/');
	path += name;
	if(name.length() != 0 && name[name.length() - 1] != '/') {
		path += '/';
	}
	return path;
}

String Directory::getParent() const
{
	if(name.length() == 0 || name == "/") {
		return nullptr;
	}
	String path('/');
	int i = name.lastIndexOf('/');
	if(i >= 0) {
		path.concat(name.c_str(), i);
	}
	return path;
}

bool Directory::next()
{
	GET_FS(false)

	int err = fs->readdir(dir, dirStat);
	if(check(err)) {
		totalSize += dirStat.size;
		++currentIndex;
		return true;
	}

	if(err != Error::NoMoreFiles) {
		debug_w("Directory '%s' read error: %s", name.c_str(), getErrorString(err).c_str());
	}

	return false;
}

} // namespace IFS
