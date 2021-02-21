/*
 * Hybrid.cpp
 *
 *  Created on: 22 Jul 2018
 *      Author: mikee47
 *
 * For testing hybrid file system
 */

#include <FsTest.h>

#include <IFS/Host/FileSystem.h>
#include <IFS/HYFS/FileSystem.h>
#include <Storage/FileDevice.h>
#include <Storage/ProgMem.h>

namespace
{
// We mount SPIFFS on a file so we can inspect if necessary
DEFINE_FSTR(SPIFFS_IMGFILE, "out/spiffs.bin")

// Partition names
DEFINE_FSTR(FS_PART_FWFS1, "fwfs1")
DEFINE_FSTR(FS_PART_FWFS2, "fwfs2")

IMPORT_FSTR(fwfsImage1, PROJECT_DIR "/out/fwfsImage1.bin")

// Flash Filesystem parameters
#define FFS_FLASH_SIZE (2 * 1024 * 1024)

// Arbitrary limit on file size
#define FWFILE_MAX_SIZE 0x100000

auto& hostfs{IFS::Host::fileSystem};

} // namespace

class HybridTest : public TestGroup
{
public:
	HybridTest() : TestGroup(_F("Hybrid tests"))
	{
	}

	void execute() override
	{
		Serial.println();

		auto part = createFwfsPartition(fwfsImage1);
		auto fs = initFWFS(part, Flag::hybrid);
		CHECK(fs != nullptr);
		if(fs != nullptr) {
			fstest(fs, Flag::recurse | Flag::readFileTest | Flag::writeThroughTest);

			auto backup_fwfs{"backup.fwfs.bin"};

			Serial.println();
			auto fs2 = initFWFSOnFile(*fs, backup_fwfs, 0);
			if(fs2 != nullptr) {
				fstest(fs2, Flag::recurse | Flag::readFileTest);
				delete fs2;
			}

			destroyStorageDevice(backup_fwfs);
			delete fs;
		}

		Serial.println();
		fs = initSpiffs(SPIFFS_IMGFILE, FFS_FLASH_SIZE);
		if(fs != nullptr) {
			fstest(fs, Flag::recurse | Flag::readFileTest);
			delete fs;
		}
		destroyStorageDevice(SPIFFS_IMGFILE);

		listPartitions();
		listDevices();
	}

	Storage::Partition createSpiffsPartition(const String& imgfile, size_t size)
	{
		Storage::Partition part;

		auto file = hostfs.open(imgfile, File::Create | File::ReadWrite);
		debug_ifs(&hostfs, file, "open('%s'", imgfile.c_str());

		if(file < 0) {
			TEST_ASSERT(false);
		} else {
			size_t curSize = hostfs.getSize(file);
			if(curSize < FFS_FLASH_SIZE) {
				hostfs.truncate(file, FFS_FLASH_SIZE);
			}
			auto dev = new Storage::FileDevice(imgfile, hostfs, file);
			Storage::registerDevice(dev);
			if(curSize < FFS_FLASH_SIZE) {
				dev->erase_range(curSize, FFS_FLASH_SIZE - curSize);
			}
			part = dev->createPartition("spiffs", Storage::Partition::SubType::Data::spiffs, 0, FFS_FLASH_SIZE);
		}

		return part;
	}

	Storage::Partition createFwfsPartition(const FlashString& image)
	{
		return Storage::progMem.createPartition(FS_PART_FWFS1, image, Storage::Partition::SubType::Data::fwfs);
	}

	Storage::Partition createFwfsPartition(IFileSystem& fileSys, const String& imgfile)
	{
		Storage::Partition part;

		auto file = fileSys.open(imgfile, File::ReadOnly);
		debug_ifs(&hostfs, file, "open('%s'", imgfile.c_str());
		if(file < 0) {
			TEST_ASSERT(false);
		} else {
			auto dev = new Storage::FileDevice(imgfile, fileSys, file);
			Storage::registerDevice(dev);

			part = dev->createPartition(FS_PART_FWFS1, Storage::Partition::SubType::Data::fwfs, 0, dev->getSize(),
										Storage::Partition::Flag::readOnly);
		}

		return part;
	}

	bool destroyStorageDevice(const String& devname)
	{
		auto dev = Storage::findDevice(devname);
		if(dev == nullptr) {
			debug_w("Device '%s' not found", devname.c_str());
			return false;
		}

		debug_i("Destroying device '%s'", devname.c_str());
		delete dev;
		return true;
	}

	int copyfile(IFileSystem* dst, IFileSystem* src, const FileStat& stat)
	{
		if(stat.attr[File::Attribute::Directory]) {
			return IFS::Error::NotSupported;
		}

		int res = FS_OK;
		IFileSystem* fserr{};

		File::Handle srcfile = src->fopen(stat, File::OpenFlag::Read);
		if(srcfile < 0) {
			res = srcfile;
			fserr = src;
		} else {
			File::Handle dstfile = dst->open(stat.name, File::CreateNewAlways | File::OpenFlag::Write);
			if(dstfile < 0) {
				res = dstfile;
				fserr = dst;
			} else {
				uint8_t buffer[512];
				for(;;) {
					res = src->read(srcfile, buffer, sizeof(buffer));
					if(res <= 0) {
						fserr = src;
						break;
					}
					int nread = res;

					res = dst->write(dstfile, buffer, nread);
					if(res < 0) {
						fserr = dst;
						break;
					}
				}
				dst->close(dstfile);
			}
			src->close(srcfile);
		}

		debug_i("Copy '%s': %s", stat.name.buffer, getErrorString(fserr, res).c_str());

		return res;
	}

	IFileSystem* initSpiffs(const String& imgfile, size_t imgsize)
	{
		auto part = createSpiffsPartition(imgfile, imgsize);
		if(!part) {
			return nullptr;
		}
		auto ffs = new IFS::SPIFFS::FileSystem(part);

		int err = ffs->mount();
		debug_ifs(ffs, err, "mount('%s')", imgfile.c_str());
		if(err < 0) {
			delete ffs;
			TEST_ASSERT(false);
			return nullptr;
		}

		IFileSystem::Info info;
		err = ffs->getinfo(info);
		debug_ifs(ffs, err, "getinfo");
		if(err < 0) {
			delete ffs;
			TEST_ASSERT(false);
			return nullptr;
		}

		if(info.freeSpace < info.volumeSize) {
			// Filesystem is populated
			return ffs;
		}

		debug_i("SPIFFS is empty, copy some stuff from FWFS");

		// Mount source data image
		part = Storage::progMem.createPartition(FS_PART_FWFS2, fwfsImage1, Storage::Partition::SubType::Data::fwfs);
		auto fwfs = IFS::createFirmwareFilesystem(part);
		int res = fwfs->mount();
		if(res < 0) {
			debug_e("FWFS mount failed: %s", getErrorString(fwfs, err).c_str());
		} else {
			DirHandle dir;
			err = fwfs->opendir(nullptr, dir);
			if(err < 0) {
				debug_e("FWFS opendir failed: %s", getErrorString(fwfs, err).c_str());
			} else {
				FileNameStat stat;
				while((err = fwfs->readdir(dir, stat)) >= 0) {
					copyfile(ffs, fwfs, stat);
				}
				fwfs->closedir(dir);
			}
		}
		delete fwfs;

		return ffs;
	}

	IFileSystem* initFWFS(Storage::Partition part, Flags flags)
	{
		IFileSystem* fs;
		if(flags[Flag::hybrid]) {
			auto spiffsPart = createSpiffsPartition(SPIFFS_IMGFILE, FFS_FLASH_SIZE);
			if(!spiffsPart) {
				return nullptr;
			}
			fs = IFS::createHybridFilesystem(part, spiffsPart);
		} else {
			fs = IFS::createFirmwareFilesystem(part);
		}

		if(fs == nullptr) {
			return nullptr;
		}

		int err = fs->mount();
		if(err < 0) {
			debug_e("HFS mount failed: %s", getErrorString(fs, err).c_str());
			delete fs;
			return nullptr;
		}

		return fs;
	}

	IFileSystem* initFWFSOnFile(IFileSystem& fileSys, const String& imgfile, Flags flags)
	{
		auto part = createFwfsPartition(fileSys, imgfile);
		if(!part) {
			return nullptr;
		}

		return initFWFS(part, flags);
	}

	void readFileTest(IFileSystem* fs, const String& filename, const File::Stat& stat)
	{
		File::Handle file = fs->open(filename, File::OpenFlag::Read);

		if(file < 0) {
			debug_w("fopen(): %s", getErrorString(fs, file).c_str());
			TEST_ASSERT(false);
			return;
		}

		char buf[1024];
		uint32_t total{0};
		while(!fs->eof(file)) {
			int len = fs->read(file, buf, sizeof(buf));
			if(len <= 0) {
				if(len < 0) {
					debug_e("Error! %s", getErrorString(fs, len).c_str());
					TEST_ASSERT(false);
				}
				break;
			}
			total += len;
		}

		fs->close(file);

		CHECK_EQ(total, stat.size);
	}

	void writeThroughTest(IFileSystem* fs, const String& filename, const FileStat& stat)
	{
		IFS::IFileSystem::Info info;
		CHECK(stat.fs->getinfo(info) >= 0);
		if(info.type == IFS::IFileSystem::Type::SPIFFS) {
			return; // Already in SPIFFS
		}

		auto originalStat = stat;
		auto originalContent = fs->getContent(filename);

		// On the hybrid volume this will copy FW file onto SPIFFS
		IFS::IFileSystem::Info fsinfo;
		fs->getinfo(fsinfo);
		if(filename.length() > fsinfo.maxPathLength || stat.name.length > fsinfo.maxNameLength) {
			debug_w("** Skipping: File name too long for writethrough");
			return;
		}

		auto file = fs->open(filename, File::OpenFlag::Write | File::OpenFlag::Append);
		if(stat.attr[File::Attribute::ReadOnly]) {
			debug_i("** Skipping copy, '%s' is marked read-only", stat.name.buffer);
			REQUIRE(file < 0);
			return;
		}

		if(file < 0) {
			debug_ifs(fs, file, "open('%s')", stat.name.buffer);
			return;
		}

		fs->write(file, nullptr, 0);
		fs->close(file);

		file = fs->open(filename, File::ReadOnly);
		CHECK(file >= 0);
		FileStat copyStat;
		int res = fs->stat(filename, &copyStat);
		CHECK(res >= 0);

		fs->close(file);

		printFileInfo(copyStat);

		CHECK(copyStat.size == originalStat.size);
		CHECK(copyStat.attr == originalStat.attr);
		CHECK(copyStat.compression == originalStat.compression);
		CHECK(copyStat.acl == originalStat.acl);
		auto now = IFS::fsGetTimeUTC();
		CHECK(abs(copyStat.mtime - now) < 3);

		auto copyContent = fs->getContent(filename);
		REQUIRE(copyContent == originalContent);
	}

	int scandir(IFileSystem* fs, const String& path, Flags flags)
	{
		debug_i("Scanning '%s'", path.c_str());

		Vector<String> files;
		Vector<String> directories;
		{
			IFS::Directory dir(fs);
			dir.open(path);
			while(dir.next()) {
				auto name = dir.stat().name.buffer;
				if(dir.stat().attr[File::Attribute::Directory]) {
					directories.add(name);
				} else {
					files.add(name);
				}
			}
		}

		for(auto& name : files) {
			String filename = path;
			if(filename.length() != 0) {
				filename += '/';
			}
			filename += name;

			FileStat stat;
			fs->stat(filename, &stat);
			stat.name.buffer = filename.begin();
			printFileInfo(stat);

			if(flags[Flag::readFileTest]) {
				readFileTest(fs, filename, stat);
			}

			if(flags[Flag::writeThroughTest]) {
				writeThroughTest(fs, filename, stat);
			}
		}

		if(flags[Flag::recurse]) {
			for(auto& name : directories) {
				String subdir = path;
				if(path.length() != 0) {
					subdir += '/';
				}
				subdir += name;
				scandir(fs, subdir, flags);
			}
		}

		return FS_OK;
	}

	void fstest(IFileSystem* fs, Flags flags)
	{
		Serial.println();
		Serial.print("FS Test: ");
		Serial.println(toString(flags));

		printFsInfo(fs);

		int res = fs->check();
		debug_i("check(): %s", getErrorString(fs, res).c_str());
		scandir(fs, "", flags);
		Serial.println();

		if(flags[Flag::writeThroughTest]) {
			Serial.println("Re-list files");
			scandir(fs, "", flags - Flag::writeThroughTest);
		}
	}
};

void REGISTER_TEST(Hybrid)
{
	registerGroup<HybridTest>();
}
