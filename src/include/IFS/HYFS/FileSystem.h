/**
 * HybridFileSystem.h
 * Hybrid file system which layers SPIFFS over FW.
 *
 * Created on: 22 Jul 2018
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
 */

/*
 * 
 * Files on SPIFFS are used in preference to FW.
 *
 * Formatting this system wipes SPIFFS to reset to a 'factory default' state.
 *
 * Images are created using a python script.
 *
 * @todo
 * 	When a file is deleted, if it exists on FW then a zero-length file should be placed on SPIFFS
 * 	with an attribute to indicate it has been deleted.
 * 	An 'undelete' could be added which would remove this to restore the FW file. It probably wouldn't
 * 	be useful on any other filing system though, and may not always succeed.
 *
 */

#pragma once

#include "../FWFS/FileSystem.h"
#include "../SPIFFS/FileSystem.h"
#include "../Types.h"

#ifndef HYFS_HIDE_FLAGS
#define HYFS_HIDE_FLAGS 1
#endif

#if HYFS_HIDE_FLAGS == 1
#include "WVector.h"
#endif

namespace IFS
{
namespace HYFS
{
class FileSystem : public IFileSystem
{
public:
	FileSystem(IObjectStore* fwStore, Storage::Partition ffsPartition) : fwfs(fwStore), ffs(ffsPartition)
	{
	}

	// IFileSystem methods
	int mount() override;
	int getinfo(Info& info) override;
	String getErrorString(int err) override;
	int opendir(const char* path, DirHandle& dir) override;
	int readdir(DirHandle dir, Stat& stat) override;
	int rewinddir(DirHandle dir) override;
	int closedir(DirHandle dir) override;
	int mkdir(const char* path) override;
	int stat(const char* path, Stat* stat) override;
	int fstat(FileHandle file, Stat* stat) override;
	int fcontrol(FileHandle file, ControlCode code, void* buffer, size_t bufSize) override;
	int setfattrtag(FileHandle file, AttributeTag tag, const void* data, size_t size) override;
	int getfattrtag(FileHandle file, AttributeTag tag, void* buffer, size_t size) override;
	int setattrtag(const char* path, AttributeTag tag, const void* data, size_t size) override;
	int getattrtag(const char* path, AttributeTag tag, void* buffer, size_t size) override;
	FileHandle open(const char* path, OpenFlags flags) override;
	FileHandle fopen(const Stat& stat, OpenFlags flags) override;
	int close(FileHandle file) override;
	int read(FileHandle file, void* data, size_t size) override;
	int write(FileHandle file, const void* data, size_t size) override;
	int lseek(FileHandle file, int offset, SeekOrigin origin) override;
	int eof(FileHandle file) override;
	int32_t tell(FileHandle file) override;
	int ftruncate(FileHandle file, size_t new_size) override;
	int flush(FileHandle file) override;
	int rename(const char* oldpath, const char* newpath) override;
	int remove(const char* path) override;
	int fremove(FileHandle file) override;
	int format() override;
	int check() override;

private:
	int hideFWFile(const char* path, bool hide);
	bool isFWFileHidden(const Stat& fwstat);

private:
	FWFS::FileSystem fwfs;
	SPIFFS::FileSystem ffs;
#if HYFS_HIDE_FLAGS == 1
	Vector<FileID> hiddenFwFiles;
#endif
};

} // namespace HYFS

} // namespace IFS
