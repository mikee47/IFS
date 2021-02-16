/*
 * FsBase.h - common base for file system classes
 *
 ****/

#pragma once

#include "FileSystem.h"

namespace IFS
{
class FsBase
{
public:
	FsBase(IFileSystem* filesys) : fileSystem(filesys)
	{
	}

	/** @brief determine if an error occurred during operation
	 *  @retval int filesystem error code
	 */
	int getLastError()
	{
		return lastError;
	}

	String getErrorString(int err) const
	{
		return fileSystem == nullptr ? Error::toString(err) : fileSystem->getErrorString(err);
	}

	String getLastErrorString() const
	{
		return getErrorString(lastError);
	}

	IFileSystem* getFileSystem() const
	{
		lastError = (fileSystem == nullptr) ? Error::NoFileSystem : FS_OK;
		return fileSystem;
	}

protected:
	/** @brief Check file operation result and note error code
	 *  @param res result of fileXXX() operation to check
	 *  @retval bool true if operation was successful, false if error occurred
	 */
	bool check(int res)
	{
		if(res >= 0) {
			return true;
		}

		lastError = res;
		return false;
	}

protected:
	mutable int lastError{FS_OK};

private:
	IFileSystem* fileSystem;
};

} // namespace IFS
