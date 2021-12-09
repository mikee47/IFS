/****
 * ArchiveStream.h
 *
 * Copyright 2021 mikee47 <mike@sillyhouse.net>
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

#include <IFS/FileSystem.h>
#include <IFS/FsBase.h>
#include "ObjectBuffer.h"
#include "BlockEncoder.h"

namespace IFS
{
namespace FWFS
{
/**
 * @brief Supports direct streaming into FWFS archive format
 *
 * Data needs to be enumerated so that all child files and directories are written
 * first. As this happens, the parent directory is built as a list of object references.
 *
 * The size of all child objects must be known before the containing object is written out.
 * It should be sufficient to buffer all of these into an internal stream.
 * This would only be a problem if large child objects are used. This is possible,
 * but the builder avoids doing it and we should too.
 *
 * Top-level object is a Volume, below that is the root Directory.
 * That means objects are written in the order:
 *      child files/directories
 *      Root directory
 *      Volume
 *      End marker
 *
 */
class ArchiveStream : public FsBase, public IDataSourceStream
{
	struct DirInfo;

public:
	enum class Flag {
		IncludeMountPoints, ///< Set to include mountpoints in archive
	};

	using Flags = BitSet<uint8_t, Flag, 1>;

	struct VolumeInfo {
		String name;			   ///< Volume Name
		uint32_t id{0};			   ///< File system ID to store
		TimeStamp creationTime{0}; ///< Volume creation time, default is current system time (UTC)

		VolumeInfo& operator=(const IFileSystem::Info& fsinfo)
		{
			name = fsinfo.name.c_str();
			id = fsinfo.volumeID;
			creationTime = fsinfo.creationTime;
			return *this;
		}
	};

	/**
	 * @brief Passed to callbacks to allow modification of output data
	 */
	class FileInfo
	{
	public:
		FileInfo(ArchiveStream& stream, DirInfo& dir, FileHandle handle, const Stat& stat)
			: handle(handle), stat(stat), stream(stream), dir(dir)
		{
		}

		FileSystem* getFileSystem() const
		{
			return stream.getFileSystem();
		}

		/**
		 * @brief Set an additional attribute on the file
		 * 
		 * These are written out before existing file metadata is copied, so will
		 * take priority. For example, if we set the compression attribute here then
		 * that is the one which the filesystem will use when mounted. However, the
		 * original compression attribute will still be present which may be helpful.
		 */
		int setAttribute(AttributeTag tag, const void* data, size_t size);

		int setAttribute(AttributeTag tag, const String& data)
		{
			return setAttribute(tag, data.c_str(), data.length());
		}

		/**
		 * @brief Set an additional user attribute
		 */
		template <typename... ParamTypes> int setUserAttribute(uint8_t tagValue, ParamTypes... params)
		{
			return setAttribute(getUserAttributeTag(tagValue), params...);
		}

		const FileHandle handle;
		const Stat& stat;

	private:
		ArchiveStream& stream;
		DirInfo& dir;
	};

	using FilterStatCallback = Delegate<bool(const Stat& stat)>;
	using CreateEncoderCallback = Delegate<IBlockEncoder*(FileInfo& file)>;

	/**
	 * @brief Construct an archive stream
	 * @param fileSystem The filesystem to read
	 * @param rootPath Where to root the generated filesystem
	 * @param flags
	 */
	ArchiveStream(FileSystem* fileSystem, VolumeInfo volumeInfo, String rootPath = nullptr, Flags flags = 0)
		: FsBase(fileSystem), currentPath(rootPath), volumeInfo(volumeInfo), flags(flags)
	{
	}

	~ArchiveStream()
	{
		reset();
	}

	/**
	 * @brief Override this method to filter items
	 * @param stat The current item
	 * @retval bool Return true to process the item, false to skip it
	 * 
	 * Use to omit temporary files or directories.
	 */
	virtual bool filterStat(const Stat& stat)
	{
		return filterStatCallback ? filterStatCallback(stat) : true;
	}

	void onFilterStat(FilterStatCallback callback)
	{
		filterStatCallback = callback;
	}

	/**
	 * @brief Override this method to implement custom encoding such as compression or encryption
	 * @param file Details of the file being archived
	 * @retval IBlockEncoder* Stream to use for file content. Return nullptr for default behaviour.
	 *
	 * To support compression or encryption, this method can create the appropriate stream
	 * type and set the appropriate attributes using methods of `FileInfo`.
	 */
	virtual IBlockEncoder* createEncoder(FileInfo& file)
	{
		return createEncoderCallback ? createEncoderCallback(file) : nullptr;
	}

	void onCreateEncoder(CreateEncoderCallback callback)
	{
		createEncoderCallback = callback;
	}

	/**
	 * @brief Get the current path being processed
	 */
	const String& getCurrentPath() const
	{
		return currentPath;
	}

	uint16_t readMemoryBlock(char* data, int bufSize) override;

	int seekFrom(int offset, SeekOrigin origin) override;

	bool isFinished() override
	{
		return state >= State::done;
	}

	MimeType getMimeType() const override
	{
		return MimeType::BINARY;
	}

	bool isSuccess() const
	{
		return state == State::done;
	}

	/**
	 * @brief Reset stream to beginning
	 */
	void reset();

private:
	enum class State {
		idle,		  ///< Not yet started
		start,		  ///< Start marker queued
		dataHeader,   ///< Header for data blob queued
		dataContent,  ///< Data blob queued
		fileHeader,   ///< Header for file queued
		dirHeader,	///< Header for directory queued
		volumeHeader, ///< Header for volume and other end objects queued
		end,		  ///< End marker queued
		done,		  ///< Finished successfully
		error,		  ///< Unsuccessful
	};

	struct DirInfo {
		DirHandle handle;
		std::unique_ptr<ObjectBuffer> content; // Directory or object content
		Object::Type type;					   // Differentiate between e.g. Directory and MountPoint
		uint8_t namelen;					   // Used to track current path

		void reset()
		{
			assert(handle == nullptr);
			namelen = 0;
			handle = nullptr;
			content.reset();
		}

		void createContent()
		{
			content.reset(new ObjectBuffer);
		}

		int addAttribute(AttributeTag tag, const void* data, size_t size);
	};

	bool fillBuffers();
	void queueStream(IDataSourceStream* stream, State newState);
	bool openRootDirectory();
	void openDirectory(const Stat& stat);
	bool readDirectory();
	bool readFileEntry(const Stat& stat);
	void sendDataHeader();
	void sendDataContent();
	void sendFileHeader();
	int getAttributes(FileHandle file, DirInfo& entry);
	void closeDirectory();
	void getVolume();

	String currentPath;
	VolumeInfo volumeInfo;
	FilterStatCallback filterStatCallback;
	CreateEncoderCallback createEncoderCallback;
	ObjectBuffer buffer;
	std::unique_ptr<IBlockEncoder> encoder;
	IDataSourceStream* dataBlock{nullptr};
	IDataSourceStream* source{nullptr};
	unsigned level{0}; ///< Directory nesting level
	static constexpr size_t maxLevels{16};
	DirInfo directories[maxLevels]{};
	uint32_t streamOffset{0}; ///< Current object ID
	uint32_t queuedSize{0};
	Flags flags{};
	State state{};
};

} // namespace FWFS
} // namespace IFS
