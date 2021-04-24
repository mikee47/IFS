/*
 * Archive.cpp
 *
 *  Created on: 22 April 2021
 *      Author: mikee47
 */

#include <FsTest.h>
#include <IFS/Helpers.h>
#include <IFS/FWFS/ArchiveStream.h>
#include <Storage/FileDevice.h>
#include <LittleFS.h>
#include <Data/Buffer/CircularBuffer.h>
#include <Data/Stream/LimitedMemoryStream.h>

DEFINE_FSTR_LOCAL(LFS_ARCHIVE_BIN, "archive-lfs.bin")
DEFINE_FSTR_LOCAL(LFS_ARCHIVE_FILTERED_BIN, "archive-lfs-filtered.bin")
DEFINE_FSTR_LOCAL(FWFS_ARCHIVE_BIN, "archive-fwfs.bin")

using ArchiveStream = IFS::FWFS::ArchiveStream;

namespace
{
class XorEncoder : public IFS::FWFS::IBlockEncoder
{
public:
	XorEncoder(ArchiveStream::FileInfo& file) : fileSystem(file.getFileSystem()), file(file.handle)
	{
		auto fileattr = file.stat.attr | FileAttribute::Encrypted;
		file.setAttribute(IFS::AttributeTag::FileAttributes, &fileattr, sizeof(fileattr));
		file.setUserAttribute(255, F("SimpleXor"));
	}

	~XorEncoder()
	{
		fileSystem->close(file);
	}

	IDataSourceStream* getNextStream() override
	{
		int size = fileSystem->read(file, buffer, sizeof(buffer));
		if(size <= 0) {
			return nullptr;
		}

		// 'encrypt' the data
		for(int i = 0; i < size; ++i) {
			buffer[i] ^= 0xAA;
		}

		stream.reset(new LimitedMemoryStream(buffer, size, size, false));
		return stream.get();
	}

private:
	IFS::FileSystem* fileSystem;
	FileHandle file;
	std::unique_ptr<LimitedMemoryStream> stream;
	uint8_t buffer[255];
};

// Return false to skip this item
bool filterStat(const IFS::Stat& stat)
{
	auto skip = [&]() {
		debug_i("Skipping %s", stat.name.c_str());
		return false;
	};

	if(stat.name.length >= 32) {
		return skip();
	}

	if(stat.isDir()) {
		return skip();
	}

	auto mimeType = ContentType::fromFullFileName(stat.name.c_str(), MIME_UNKNOWN);
	switch(mimeType) {
	case MIME_JS:
	case MIME_PNG:
		return skip();
	default:
		return true;
	}
}

} // namespace

class ArchiveTest : public TestGroup
{
public:
	ArchiveTest() : TestGroup(_F("FWFS Archive Stream"))
	{
	}

	void execute() override
	{
		REQUIRE(spiffs_mount());

		ArchiveStream::VolumeInfo volumeInfo;
		volumeInfo.id = 0xdeadbeef;

		auto lfsPart = Storage::findDefaultPartition(Storage::Partition::SubType::Data::littlefs);
		auto lfs = IFS::createLfsFilesystem(lfsPart);
		CHECK(lfs != nullptr);
		REQUIRE(lfs->mount() == FS_OK);
		volumeInfo.name = F("Full backup of LFS filesystem");
		backupFilesystem(lfs, volumeInfo, LFS_ARCHIVE_BIN);
		volumeInfo.name = F("Backup of LFS filesystem with filtering and XORed file content");
		backupFilesystem(lfs, volumeInfo, LFS_ARCHIVE_FILTERED_BIN, filterStat, [](ArchiveStream::FileInfo& file) {
			file.setAttribute(IFS::AttributeTag::Comment, F("This is a file comment"));
			return new XorEncoder(file);
		});
		delete lfs;

		// Create an archive which should be byte-identical to original image
		auto fwfsPart = Storage::findDefaultPartition(Storage::Partition::SubType::Data::fwfs);
		auto fwfs = IFS::createFirmwareFilesystem(fwfsPart);
		CHECK(fwfs != nullptr);
		CHECK(fwfs->mount() == FS_OK);
		IFS::FileSystem::NameInfo fsinfo;
		int err = fwfs->getinfo(fsinfo);
		CHECK(err >= 0);
		volumeInfo = fsinfo;
		backupFilesystem(fwfs, volumeInfo, FWFS_ARCHIVE_BIN);
		delete fwfs;

		// Verify that the generated image is identical to the source image
		TEST_CASE("Compare images")
		{
			// List the main root directory to show the filesystem image
			listdir(getFileSystem(), nullptr, 0);

			File f;
			REQUIRE(f.open(FWFS_ARCHIVE_BIN));
			uint32_t offset{0};
			int res = f.readContent([&](const char* buffer, size_t size) -> int {
				char original[size];
				CHECK(fwfsPart.read(offset, original, size));
				if(memcmp(original, buffer, size) != 0) {
					debug_e("Failed at offset 0x%08x", offset);
					TEST_ASSERT(false);
				}
				offset += size;
				return size;
			});
			REQUIRE(res == int(f.getSize()));
		}
	}

	void backupFilesystem(IFS::FileSystem* fs, const ArchiveStream::VolumeInfo& volumeInfo, const String& filename,
						  ArchiveStream::FilterStatCallback filterStat = nullptr,
						  ArchiveStream::CreateEncoderCallback createEncoder = nullptr)
	{
		char name[64];
		IFS::FileSystem::Info info{name, sizeof(name)};
		CHECK(fs->getinfo(info) == FS_OK);
		m_printf(_F("\r\n\nBacking up %s filesystem '%s' to '%s'\r\n"), toString(info.type).c_str(), name,
				 filename.c_str());
		printFsInfo(fs);
		listdir(fs, nullptr, Flag::attributes);
		ArchiveStream archive(fs, volumeInfo);
		archive.onFilterStat(filterStat);
		archive.onCreateEncoder(createEncoder);
		FileStream stream;
		stream.open(filename, File::CreateNewAlways | File::WriteOnly);
		stream.copyFrom(&archive);
		stream.close();
		REQUIRE(archive.isSuccess());

		// Force failure
		archive.reset();
		CircularBuffer buffer(16);
		buffer.copyFrom(&archive);
		REQUIRE(!archive.isSuccess());

#ifdef ARCH_HOST
		// Make copy of archive in current directory for post-mortem
		auto& hostfs = IFS::Host::getFileSystem();
		FileStream src;
		CHECK(src.open(filename));
		IFS::FileStream dst(&hostfs);
		CHECK(dst.open(filename, File::CreateNewAlways | File::WriteOnly));
		dst.copyFrom(&src);
#endif

		TEST_CASE("Verify archive image")
		{
			listImage(filename);
		}
	}

	void listImage(const String& filename)
	{
		m_puts("\r\n");
		m_puts("\r\n");
		auto file = fileOpen(filename, File::ReadOnly);
		auto dev = new Storage::FileDevice(filename, *getFileSystem(), file);
		Storage::registerDevice(dev);
		auto part = dev->createPartition("archive", Storage::Partition::SubType::Data::fwfs, 0, dev->getSize());
		auto fs = IFS::createFirmwareFilesystem(part);
		CHECK(fs != nullptr);
		REQUIRE(fs->mount() >= 0);
		printFsInfo(fs);
		listdir(fs, nullptr, Flag::attributes | Flag::recurse);
		delete fs;
		delete dev;
	}
};

void REGISTER_TEST(Archive)
{
	registerGroup<ArchiveTest>();
}
