/****
 * FileCopier.h
 *
 * Created August 2018 by mikee471
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

#include "include/IFS/FileCopier.h"
#include "include/IFS/Directory.h"

namespace
{
String abspath(const String& path, const char* name)
{
	String s;
	if(path.length() != 0) {
		s += path;
		s += '/';
	}
	return s + name;
}

} // namespace

String toString(IFS::FileCopier::Operation operation)
{
	using Operation = IFS::FileCopier::Operation;
	switch(operation) {
#define XX(tag)                                                                                                        \
	case(Operation::tag):                                                                                              \
		return F(#tag);
		IFS_FILECOPIER_OPERATION_MAP(XX)
#undef XX
	default:
		return nullptr;
	}
}

namespace IFS
{
bool FileCopier::handleError(FileSystem& fileSys, int errorCode, Operation operation, const String& path)
{
	if(errorHandler) {
		return errorHandler(fileSys, errorCode, operation, path);
	}

	debug_e("%s('%s') failed: %s", toString(operation).c_str(), path.c_str(),
			fileSys.getErrorString(errorCode).c_str());
	return false;
}

bool FileCopier::copyFile(const String& srcFileName, const String& dstFileName)
{
	debug_d("copyFile('%s', '%s')", srcFileName.c_str(), dstFileName.c_str());
	File srcFile(&srcfs);
	if(!srcFile.open(srcFileName)) {
		return handleError(srcFile, Operation::open, srcFileName);
	}
	File dstFile(&dstfs);
	if(!dstFile.open(dstFileName, File::CreateNewAlways | File::WriteOnly)) {
		return handleError(dstFile, Operation::create, dstFileName);
	}
	srcFile.readContent([&dstFile](const char* buffer, size_t size) -> int { return dstFile.write(buffer, size); });
	int err = dstFile.getLastError();
	if(err < 0) {
		return handleError(dstFile, Operation::write, dstFileName);
	}
	err = srcFile.getLastError();
	if(err < 0) {
		return handleError(srcFile, Operation::read, srcFileName);
	}

	return copyAttributes(srcFile, dstFile, srcFileName, dstFileName);
}

bool FileCopier::copyAttributes(const String& srcPath, const String& dstPath)
{
	debug_d("copyAttributes('%s', '%s')", srcPath.c_str(), dstPath.c_str());
	File src(&srcfs);
	if(!src.open(srcPath)) {
		return handleError(src, Operation::open, srcPath);
	}
	File dst(&dstfs);
	if(!dst.open(dstPath, File::WriteOnly)) {
		return handleError(dst, Operation::create, dstPath);
	}

	return copyAttributes(src, dst, srcPath, dstPath);
}

bool FileCopier::copyAttributes(File& src, File& dst, const String& srcPath, const String& dstPath)
{
	auto callback = [&](AttributeEnum& e) -> bool {
		debug_d("setAttribute(%u, %s)", unsigned(e.tag), toString(e.tag).c_str());
		debug_hex(DBG, "ATTR", e.buffer, e.size);
		return dst.setAttribute(e.tag, e.buffer, e.size);
	};
	char buffer[1024];
	src.enumAttributes(callback, buffer, sizeof(buffer));
	if(dst.getLastError() < 0) {
		return handleError(dst, Operation::setattr, dstPath);
	}
	if(src.getLastError() < 0) {
		return handleError(src, Operation::enumattr, srcPath);
	}

	return true;
}

bool FileCopier::copyDir(const String& srcPath, const String& dstPath)
{
	if(!copyAttributes(srcPath, dstPath)) {
		return false;
	}

	Directory srcDir(&srcfs);
	if(!srcDir.open(srcPath)) {
		return false;
	}

	struct Dir {
		CString name;
		TimeStamp mtime;
	};
	Vector<Dir> directories;

	while(srcDir.next()) {
		auto& stat = srcDir.stat();
		if(stat.isDir()) {
			// Don't copy mount points
			if(!stat.attr[FileAttribute::MountPoint]) {
				directories.add({stat.name.c_str(), stat.mtime});
			}
			continue;
		}

		String srcFileName = abspath(srcPath, stat.name.c_str());
		String dstFileName = abspath(dstPath, stat.name.c_str());
		if(!copyFile(srcFileName, dstFileName)) {
			return false;
		}
	}

	srcDir.close();

	auto time = fsGetTimeUTC();

	for(auto& dir : directories) {
		String dstDirPath = abspath(dstPath, dir.name.c_str());
		int err = dstfs.mkdir(dstDirPath);
		if(err < 0 && !handleError(dstfs, err, Operation::mkdir, dstDirPath)) {
			return false;
		}
		if(dir.mtime != time) {
			dstfs.settime(dstDirPath, dir.mtime);
		}
		String srcDirPath = abspath(srcPath, dir.name.c_str());
		if(!copyDir(srcDirPath, dstDirPath)) {
			return false;
		}
	}

	return true;
}

} // namespace IFS
