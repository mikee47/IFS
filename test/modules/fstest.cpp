/*
 * main.cpp
 *
 *  Created on: 22 Jul 2018
 *      Author: mikee47
 *
 * For testing file systems
 */

#include <SmingTest.h>

#include <hostlib/CommandLine.h>
#include <IFS/Host/FileSystem.h>
#include <IFS/HYFS/FileSystem.h>
#include <IFS/Helpers.h>
#include <Storage.h>
#include <Storage/SpiFlash.h>
#include <Storage/FileDevice.h>
#include <Storage/ProgMem.h>

namespace
{
// We mount SPIFFS on a file so we can inspect if necessary
DEFINE_FSTR(FLASHMEM_DMP, "out/flashmem.dmp")

// Partition names
DEFINE_FSTR(FS_PART_FWFS1, "fwfs1")
DEFINE_FSTR(FS_PART_FWFS2, "fwfs2")

IMPORT_FSTR(fwfsImage1, PROJECT_DIR "/out/fwfsImage1.bin")
//IMPORT_FSTR(fwfsImage2, PROJECT_DIR "/out/fwfsImage2.bin")

#define FLAG_MAP(XX)                                                                                                   \
	XX(hybrid, "Use hybrid filesystem, enabling write tests")                                                          \
	XX(readFileTest, "Read content of every file")                                                                     \
	XX(writeThroughTest, "With hybrid filesystem, 'touch' files to propagate them to SPIFFS from FWFS")

enum class Flag {
#define XX(name, desc) name,
	FLAG_MAP(XX)
#undef XX
};

using Flags = BitSet<uint32_t, Flag, 3>;

String toString(Flag f)
{
	switch(f) {
#define XX(name, desc)                                                                                                 \
	case Flag::name:                                                                                                   \
		return F(#name);
		FLAG_MAP(XX)
#undef XX
	default:
		return nullptr;
	}
}

// Flash Filesystem parameters
#define FFS_FLASH_SIZE (2048 * 1024)

// Arbitrary limit on file size
#define FWFILE_MAX_SIZE 0x100000

} // namespace

using IFileSystem = IFS::IFileSystem;

class FsTest : public TestGroup
{
public:
	FsTest() : TestGroup(_F("IFS")), hostFileSys(gHostFileSystem)
	{
	}

	Storage::Partition createSpiffsPartition(const String& imgfile, size_t size)
	{
		Storage::Partition part;

		auto file = hostFileSys.open(imgfile, File::Create | File::ReadWrite);
		if(file < 0) {
			debug_e("Failed to open '%s': %s", imgfile.c_str(), hostFileSys.getErrorString(file).c_str());
		} else {
			size_t curSize = hostFileSys.getSize(file);
			if(curSize < FFS_FLASH_SIZE) {
				hostFileSys.truncate(file, FFS_FLASH_SIZE);
			}
			auto dev = new Storage::FileDevice(imgfile, hostFileSys, file);
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
		if(file < 0) {
			debug_e("Failed to open '%s': %s", imgfile.c_str(), fileSys.getErrorString(file).c_str());
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

	String getErrorString(IFileSystem* fs, int err)
	{
		if(fs == nullptr) {
			return IFS::Error::toString(err);
		} else {
			return fs->getErrorString(err);
		}
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
		if(err < 0) {
			debug_e("SPIFFS mount failed: %s", getErrorString(ffs, err).c_str());
			delete ffs;
			return nullptr;
		}

		IFileSystem::Info info;
		err = ffs->getinfo(info);
		if(err < 0) {
			debug_e("Failed to get SPIFFS info: %s", getErrorString(ffs, err).c_str());
			delete ffs;
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

	IFileSystem* initFWFSOnFile(IFileSystem& fileSys, const String& imgfile)
	{
		auto part = createFwfsPartition(fileSys, imgfile);
		if(!part) {
			return nullptr;
		}

		return IFS::createFirmwareFilesystem(part);
	}

	IFileSystem* initFWFS(Storage::Partition part)
	{
		IFileSystem* fs;
		if(flags[Flag::hybrid]) {
			auto spiffsPart = createSpiffsPartition(FLASHMEM_DMP, FFS_FLASH_SIZE);
			if(part == nullptr) {
				return nullptr;
			}
			fs = IFS::createHybridFilesystem(part, spiffsPart);
		} else {
			fs = IFS::createFirmwareFilesystem(part);
		}

		int err = fs->mount();
		if(err < 0) {
			debug_e("HFS mount failed: %s", getErrorString(fs, err).c_str());
			delete fs;
			fs = nullptr;
		}

		return fs;
	}

	void printFsInfo(IFileSystem* fs)
	{
		char namebuf[256];
		IFileSystem::Info info(namebuf, sizeof(namebuf));
		int res = fs->getinfo(info);
		if(res < 0) {
			debug_e("fileSystemGetInfo(): %s", getErrorString(fs, res).c_str());
			return;
		}

		debug_i("type:       %s", toString(info.type).c_str());
		debug_i("partition:  %s", info.partition.name().c_str());
		//	if(info.media) {
		//		debug_i("mediaType:  %s", toString(info.media->type()).c_str());
		//		debug_i("bus:        %s", toString(info.media->bus()).c_str());
		//	}
		debug_i("attr:       %s", toString(info.attr).c_str());
		debug_i("volumeID:   0x%08X", info.volumeID);
		debug_i("name:       %s", info.name.buffer);
		debug_i("volumeSize: %u", info.volumeSize);
		debug_i("freeSpace:  %u", info.freeSpace);
	}

	void printFileInfo(const FileStat& stat)
	{
		IFileSystem::Info info;
		stat.fs->getinfo(info);
		debug_i("%-50s %6u %6u %s #0x%04x %s %s %s %s", stat.name.buffer, stat.size, stat.originalSize,
				toString(info.type).c_str(), stat.id, toString(stat.acl).c_str(), toString(stat.attr).c_str(),
				toString(stat.compression).c_str(), timeToStr(stat.mtime, " ").c_str());
	}

	// Displays as local time
	String timeToStr(time_t t, const char* dtsep)
	{
		struct tm* tm = localtime(&t);
		char buffer[64];
		m_snprintf(buffer, sizeof(buffer), "%02u/%02u/%04u%s%02u:%02u:%02u", tm->tm_mday, tm->tm_mon + 1,
				   1900 + tm->tm_year, dtsep, tm->tm_hour, tm->tm_min, tm->tm_sec);
		return buffer;
	}

	bool readFileTest(const FileStat& stat)
	{
		auto fs{stat.fs};

		File::Handle file = fs->fopen(stat, File::OpenFlag::Read);

		if(file < 0) {
			debug_w("fopen(): %s", getErrorString(fs, file).c_str());
			return false;
		}

		int result{true};
		char buf[1024];
		uint32_t total{0};
		while(!fs->eof(file)) {
			int len = fs->read(file, buf, sizeof(buf));
			if(len <= 0) {
				if(len < 0) {
					debug_e("Error! %s", getErrorString(fs, len).c_str());
					result = false;
				}
				break;
			}
			//			if(total == 0)
			//			m_printHex("data", buf, len); //std::min(len, 32));
			total += len;
		}
		fs->close(file);

		if(total != stat.size) {
			debug_e("Size mismatch: stat reports %u bytes, read %u bytes", stat.size, total);
			result = false;
		}

		return result;
	}

	int scandir(IFileSystem* fs, const String& path)
	{
		debug_i("Scanning '%s'", path.c_str());

		DirHandle dir{};
		int res = fs->opendir(path.c_str(), dir);
		if(res < 0) {
			return res;
		}

		FileNameStat stat;
		while((res = fs->readdir(dir, stat)) >= 0) {
			printFileInfo(stat);

			if(flags[Flag::readFileTest]) {
				readFileTest(stat);
			}

			if(stat.attr[File::Attribute::Directory]) {
				if(path.length() == 0) {
					scandir(fs, String(stat.name));
				} else {
					scandir(fs, path + '/' + String(stat.name));
				}
				continue;
			}

			if(flags[Flag::writeThroughTest]) {
				if(stat.attr[File::Attribute::ReadOnly]) {
					continue;
				}

				// On the hybrid volume this will copy FW file onto SPIFFS
				auto file = fs->fopen(stat, File::OpenFlag::Write | File::OpenFlag::Append);
				if(file < 0) {
					debug_e("open('%s') failed, %s", stat.name.buffer, fileGetErrorString(file).c_str());
					// TEST_ASSERT(false);
				} else {
					fs->write(file, nullptr, 0);
					fs->close(file);

					file = fs->fopen(stat, File::ReadOnly);
					REQUIRE(file >= 0);
					IFS::FileStat stat2;
					int res = fs->fstat(file, stat2);
					REQUIRE(res >= 0);
					printFileInfo(stat2);
					fs->close(file);
				}
			}
		}

		debug_i("readdir('%s'): %s", path.c_str(), getErrorString(fs, res).c_str());

		fs->closedir(dir);

		return FS_OK;
	}

	void fstest(IFileSystem* fs)
	{
		int res = fs->check();
		debug_i("check(): %s", getErrorString(fs, res).c_str());
		scandir(fs, "");


		flags[Flag::writeThroughTest] = false;
		scandir(fs, "");
	}

	void execute()
	{
		auto& params = commandLine.getParameters();

		String imgfile = params.findIgnoreCase("imgfile").getValue();
		Serial.print("imgfile = '");
		Serial.print(imgfile);
		Serial.println('\'');

		for(unsigned i = 0; i < Flags::size(); ++i) {
			Flag f{int(i)};
			flags[f] = params.findIgnoreCase(toString(f));
		}

		Serial.println();

		Storage::Partition part;
		if(imgfile) {
			part = createFwfsPartition(hostFileSys, imgfile);
		} else {
			part = createFwfsPartition(fwfsImage1);
		}

		auto fs = initFWFS(part);
		if(fs != nullptr) {
			printFsInfo(fs);
			fstest(fs);

			PSTR_ARRAY(backup_fwfs, "backup.fwfs")

			Serial.println();
			auto fs2 = initFWFSOnFile(*fs, backup_fwfs);
			if(fs2 != nullptr) {
				printFsInfo(fs2);
				fstest(fs2);
				delete fs2;
			}

			destroyStorageDevice(backup_fwfs);
			delete fs;
		}
		if(imgfile) {
			destroyStorageDevice(imgfile);
		}

		Serial.println();
		fs = initSpiffs(FLASHMEM_DMP, FFS_FLASH_SIZE);
		if(fs != nullptr) {
			printFsInfo(fs);
			fstest(fs);
			delete fs;
		}
		destroyStorageDevice(FLASHMEM_DMP);

		Serial.println();
		Serial.println(_F("Registered storage devices:"));
		for(Storage::Device* dev = Storage::spiFlash; dev != nullptr; dev = dev->getNext()) {
			Serial.print("  name = '");
			Serial.print(dev->getName());
			Serial.print(_F("', type = "));
			Serial.print(toString(dev->getType()));
			Serial.print(_F(", size = 0x"));
			Serial.println(dev->getSize(), HEX);
			for(auto part : dev->partitions()) {
				Serial.print("    Part '");
				Serial.print(part.name());
				Serial.print(_F("', type = "));
				Serial.print(part.longTypeString());
				Serial.print(_F(", address = 0x"));
				Serial.print(part.address(), HEX);
				Serial.print(_F(", size = 0x"));
				Serial.println(part.size(), HEX);
			}
		}
	}

private:
	IFS::Host::FileSystem gHostFileSystem;
	IFileSystem& hostFileSys;
	Flags flags;
};

void REGISTER_TEST(General)
{
	registerGroup<FsTest>();
}
