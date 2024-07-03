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

#pragma once

#include "FileSystem.h"
#include "File.h"

#define IFS_FILECOPIER_OPERATION_MAP(XX)                                                                               \
	XX(stat)                                                                                                           \
	XX(open)                                                                                                           \
	XX(create)                                                                                                         \
	XX(mkdir)                                                                                                          \
	XX(read)                                                                                                           \
	XX(write)                                                                                                          \
	XX(enumattr)                                                                                                       \
	XX(setattr)

namespace IFS
{
/**
 * @brief Class to manage copying of files and directories including attributes
 */
class FileCopier
{
public:
	enum class Operation {
#define XX(tag) tag,
		IFS_FILECOPIER_OPERATION_MAP(XX)
#undef XX
	};

	/**
	 * @brief Error information passed to callback
	 */
	struct ErrorInfo {
		FileSystem& fileSys;
		Operation operation;
		const String& path;
		int errorCode;
		AttributeTag attr{};

		ErrorInfo(FileSystem& fileSys, Operation operation, const String& path, int errorCode)
			: fileSys(fileSys), operation(operation), path(path), errorCode(errorCode)
		{
		}

		ErrorInfo(FsBase& obj, Operation operation, const String& path)
			: fileSys(*obj.getFileSystem()), operation(operation), path(path), errorCode(obj.getLastError())
		{
		}

		ErrorInfo(FsBase& obj, Operation operation, const String& path, AttributeTag attr)
			: ErrorInfo(obj, operation, path)
		{
			this->attr = attr;
		}

		operator String() const
		{
			String s;
			s += operation;
			s += "(\"";
			s += path;
			s += '"';
			if(operation == Operation::setattr) {
				s += ", ";
				s += attr;
			}
			s += F("): ");
			s += fileSys.getErrorString(errorCode);
			return s;
		}
	};

	/**
     * @brief Return true to ignore error and continue copying, false to stop
     */
	using ErrorHandler = Delegate<bool(const ErrorInfo& info)>;

	FileCopier(FileSystem& srcfs, FileSystem& dstfs);

	bool copyFile(const String& srcFileName, const String& dstFileName);
	bool copyDir(const String& srcPath, const String& dstPath);
	bool copyAttributes(const String& srcPath, const String& dstPath);

	void onError(ErrorHandler callback)
	{
		errorHandler = callback;
	}

private:
	bool copyFile(const String& srcPath, const String& dstPath, const Stat& stat);
	bool copyAttributes(File& src, File& dst, const String& srcPath, const String& dstPath);

	bool handleError(const ErrorInfo& info);

	FileSystem& srcfs;
	FileSystem& dstfs;
	ErrorHandler errorHandler;
	IFileSystem::Attributes dstAttr;
};

} // namespace IFS

String toString(IFS::FileCopier::Operation operation);
