/**
 * FileSystem.h
 * Provides an IFS FileSystem implementation for LittleFS.
 *
 * Created on: 8 April 2021
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

#pragma once

#include "../FileSystem.h"
#include <littlefs/lfs.h>
#include <memory>

namespace IFS
{
namespace LittleFS
{
// File handles start at this value
#ifndef LFS_HANDLE_MIN
#define LFS_HANDLE_MIN 200
#endif

// Maximum number of file descriptors
#ifndef LFS_MAX_FDS
#define LFS_MAX_FDS 5
#endif

// Maximum file handle value
#define LFS_HANDLE_MAX (LFS_HANDLE_MIN + LFS_MAX_FDS - 1)

constexpr size_t LFS_READ_SIZE{16};
constexpr size_t LFS_PROG_SIZE{16};
constexpr size_t LFS_BLOCK_SIZE{4096};
constexpr size_t LFS_BLOCK_CYCLES{500};
constexpr size_t LFS_CACHE_SIZE{16};
constexpr size_t LFS_LOOKAHEAD_SIZE{16};

/**
 * Wraps LittleFS
 */
class FileSystem : public IFileSystem
{
public:
	FileSystem(Storage::Partition partition) : partition(partition)
	{
	}

	~FileSystem();

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
	int setacl(FileHandle file, const ACL& acl) override;
	int setattr(const char* path, FileAttributes attr) override;
	int settime(FileHandle file, time_t mtime) override;
	int setcompression(FileHandle file, const Compression& compression) override;
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
	int flushMeta(FileHandle file);

	void touch(FileHandle file)
	{
		settime(file, fsGetTimeUTC());
	}

	int tryMount();
	int fillStat(const char* path, lfs_info& info, Stat& stat);

	/**
	 * @brief Identifies LFS attribute tags
	 */
	enum class AttributeTag {
		ModifiedTime,
		FileAttributes,
		Acl,
		Compression,
	};

	struct FileMeta {
		TimeStamp mtime;
		FileAttributes attr;
		ACL acl;
		Compression compression;

		void fillStat(Stat& stat) const
		{
			stat.mtime = mtime;
			stat.attr = attr;
			stat.compression = compression;
			if(compression.type != Compression::Type::None) {
				stat.attr += FileAttribute::Compressed;
			}
			stat.acl = acl;
		}
	};

	/**
	 * @brief Details for an open file
	 */
	struct FileDescriptor {
		CString name;
		lfs_file_t file{};
		FileMeta meta{};
		uint8_t buffer[LFS_CACHE_SIZE];
		struct lfs_attr attrs[4];
		struct lfs_file_config config;

		template <typename T> constexpr lfs_attr makeAttr(AttributeTag tag, T& value)
		{
			return lfs_attr{uint8_t(tag), &value, sizeof(value)};
		}

		FileDescriptor()
			: attrs{
				makeAttr(AttributeTag::ModifiedTime, meta.mtime),
				makeAttr(AttributeTag::FileAttributes, meta.attr),
				makeAttr(AttributeTag::Acl, meta.acl), 
				makeAttr(AttributeTag::Compression, meta.compression),
			  },
			  config{buffer, attrs, ARRAY_SIZE(attrs)}
		{
		}
	};

	template <typename T> int get_attr(const char* path, AttributeTag tag, T& attr)
	{
		int err = lfs_getattr(&lfs, path, uint8_t(tag), &attr, sizeof(attr));
		return Error::fromSystem(err);
	}

	template <typename T> int set_attr(const char* path, AttributeTag tag, T& attr)
	{
		int err = lfs_setattr(&lfs, path, uint8_t(tag), &attr, sizeof(attr));
		return Error::fromSystem(err);
	}

	static int f_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
	{
		auto part = static_cast<Storage::Partition*>(c->context);
		uint32_t addr = (block * LFS_BLOCK_SIZE) + off;
		return part && part->read(addr, buffer, size) ? FS_OK : Error::ReadFailure;
	}

	static int f_prog(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
	{
		auto part = static_cast<Storage::Partition*>(c->context);
		uint32_t addr = (block * LFS_BLOCK_SIZE) + off;
		return part && part->write(addr, buffer, size) ? FS_OK : Error::WriteFailure;
	}

	static int f_erase(const struct lfs_config* c, lfs_block_t block)
	{
		auto part = static_cast<Storage::Partition*>(c->context);
		uint32_t addr = block * LFS_BLOCK_SIZE;
		size_t size = LFS_BLOCK_SIZE;
		return part && part->erase_range(addr, size) ? FS_OK : Error::EraseFailure;
	}

	static int f_sync(const struct lfs_config* c)
	{
		(void)c;
		return FS_OK;
	}

	Storage::Partition partition;
	uint8_t readBuffer[LFS_CACHE_SIZE];
	uint8_t progBuffer[LFS_CACHE_SIZE];
	uint8_t lookaheadBuffer[LFS_LOOKAHEAD_SIZE];
	lfs_config config{
		.read = f_read,
		.prog = f_prog,
		.erase = f_erase,
		.sync = f_sync,
		.read_size = LFS_READ_SIZE,
		.prog_size = LFS_PROG_SIZE,
		.block_size = LFS_BLOCK_SIZE,
		.block_cycles = LFS_BLOCK_CYCLES,
		.cache_size = LFS_CACHE_SIZE,
		.lookahead_size = LFS_LOOKAHEAD_SIZE,
		.read_buffer = readBuffer,
		.prog_buffer = progBuffer,
		.lookahead_buffer = lookaheadBuffer,
	};
	lfs_t lfs{};
	std::unique_ptr<FileDescriptor> fileDescriptors[LFS_MAX_FDS];
	bool mounted{false};
}; // namespace LittleFS

} // namespace LittleFS
} // namespace IFS
