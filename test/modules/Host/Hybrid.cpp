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
#include <IFS/Helpers.h>
#include <Storage/FileDevice.h>
#include <Storage/ProgMem.h>
#include <Crypto/Md5.h>
#include <Spiffs.h>
#include <LittleFS.h>

using SubType = Storage::Partition::SubType::Data;

namespace
{
// We mount SPIFFS/LittleFS on a file so we can inspect if necessary
DEFINE_FSTR(SPIFFS_IMGFILE, "out/spiffs.bin")
DEFINE_FSTR(LFS_IMGFILE, "out/lfs.bin")

// Partition names
DEFINE_FSTR(FS_PART_FWFS1, "fwfs1")
DEFINE_FSTR(FS_PART_FWFS2, "fwfs2")

IMPORT_FSTR(fwfsImage1, PROJECT_DIR "/out/fwfsImage1.bin")

DEFINE_FSTR(BACKUP_FWFS, "backup.fwfs.bin")

// Flash Filesystem parameters
#define FFS_FLASH_SIZE (2 * 1024 * 1024)

// Arbitrary limit on file size
#define FWFILE_MAX_SIZE 0x100000

FileSystem* fwfsRef;

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
		fwfsRef = initFWFS(part, SubType::fwfs);

		TEST_CASE("Verify Hybrid SPIFFS")
		{
			verify(part, SubType::spiffs);
			destroyStorageDevice(SPIFFS_IMGFILE);
		}

		TEST_CASE("Verify Hybrid LittleFS")
		{
			verify(part, SubType::littlefs);
			destroyStorageDevice(LFS_IMGFILE);
		}

		listPartitions(Serial);
		listDevices(Serial);
	}

	void verify(Storage::Partition fwfsPart, SubType subtype)
	{
		auto fs = initFWFS(fwfsPart, subtype);
		CHECK(fs != nullptr);
		if(fs != nullptr) {
			fstest(*fs, Flag::readFileTest | Flag::writeThroughTest);

			Serial.println();

			auto part = createFwfsPartition(*fs, BACKUP_FWFS);
			auto fs2 = initFWFS(part, SubType::fwfs);
			if(fs2 != nullptr) {
				fstest(*fs2, Flag::readFileTest);
				delete fs2;
			}

			destroyStorageDevice(BACKUP_FWFS);
			delete fs;
		}

		Serial.println();
		fs = initVolume(subtype);
		if(fs != nullptr) {
			fstest(*fs, Flag::readFileTest);
			delete fs;
		}
	}

	Storage::Partition createPartition(const String& imgfile, size_t size, const String& name, SubType subtype)
	{
		auto part = Storage::findPartition(name);
		if(part) {
			return part;
		}

		auto& hostfs = IFS::Host::getFileSystem();
		auto file = hostfs.open(imgfile, File::Create | File::ReadWrite);
		debug_ifs(&hostfs, file, "open('%s')", imgfile.c_str());

		if(file < 0) {
			TEST_ASSERT(false);
			return part;
		}

		size_t curSize = hostfs.getSize(file);
		if(curSize < size) {
			hostfs.ftruncate(file, size);
		}
		auto dev = new Storage::FileDevice(imgfile, hostfs, file);
		Storage::registerDevice(dev);
		if(curSize < size) {
			dev->erase_range(curSize, size - curSize);
		}
		part = dev->createPartition(name, subtype, 0, size);
		return part;
	}

	Storage::Partition createSpiffsPartition(const String& imgfile, size_t size)
	{
		return createPartition(imgfile, size, F("spiffs"), SubType::spiffs);
	}

	Storage::Partition createFwfsPartition(const FlashString& image)
	{
		return Storage::progMem.createPartition(FS_PART_FWFS1, image, SubType::fwfs);
	}

	Storage::Partition createFwfsPartition(FileSystem& fileSys, const String& imgfile)
	{
		Storage::Partition part;

		auto file = fileSys.open(imgfile, File::ReadOnly);
		debug_ifs(&fileSys, file, "open('%s'", imgfile.c_str());
		if(file < 0) {
			TEST_ASSERT(false);
		} else {
			auto dev = new Storage::FileDevice(imgfile, fileSys, file);
			Storage::registerDevice(dev);

			part = dev->createPartition(FS_PART_FWFS1, SubType::fwfs, 0, dev->getSize(),
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

	int copyfile(FileSystem* dst, FileSystem* src, const IFS::Stat& stat)
	{
		if(stat.isDir()) {
			return IFS::Error::NotSupported;
		}

		IFS::File srcFile(src);
		IFS::File dstFile(dst);
		if(srcFile.open(stat.name) && dstFile.open(stat.name, IFS::File::CreateNewAlways | IFS::File::WriteOnly)) {
			srcFile.readContent([&dstFile](const char* buffer, size_t size) { return dstFile.write(buffer, size); });
		}

		int err = dstFile.getLastError();
		if(err < 0) {
			debug_e("Copy: write '%s' failed: %s", stat.name.buffer, dstFile.getLastErrorString().c_str());
			return err;
		}

		err = srcFile.getLastError();
		if(err < 0) {
			debug_e("Copy: read '%s' failed: %s", stat.name.buffer, srcFile.getLastErrorString().c_str());
			return err;
		}

		// Copy metadata
		auto callback = [&](IFS::AttributeEnum& e) -> bool {
			// Ignore errors here as destination file system doesn't necessarily support all attributes
			int err = dstFile.setAttribute(e.tag, e.buffer, e.size);
			if(err < 0) {
				debug_w("fsetxattr(%s): %s", toString(e.tag).c_str(), dstFile.getLastErrorString().c_str());
			}
			return true;
		};
		char buffer[1024];
		err = srcFile.enumAttributes(callback, buffer, sizeof(buffer));
		if(err < 0) {
			debug_w("fenumxattr(): %s", srcFile.getLastErrorString().c_str());
			return err;
		}

		debug_i("Copy '%s': OK", stat.name.buffer);
		return FS_OK;
	}

	FileSystem* initVolume(SubType subtype)
	{
		auto ffs = createFilesystem(subtype);
		TEST_ASSERT(ffs != nullptr);

		int err = ffs->mount();
		debug_ifs(ffs, err, "mount('%s')", toString(subtype).c_str());
		if(err < 0) {
			delete ffs;
			TEST_ASSERT(false);
			return nullptr;
		}

		FileSystem::Info info;
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

		debug_i("Filesystem is empty, copy some stuff from FWFS");

		// Mount source data image
		auto part = Storage::progMem.createPartition(FS_PART_FWFS2, fwfsImage1, SubType::fwfs);
		auto fwfs = IFS::createFirmwareFilesystem(part);
		int res = fwfs->mount();
		if(res < 0) {
			debug_e("FWFS mount failed: %s", fwfs->getErrorString(err).c_str());
		} else {
			DirHandle dir;
			err = fwfs->opendir(nullptr, dir);
			if(err < 0) {
				debug_e("FWFS opendir failed: %s", fwfs->getErrorString(err).c_str());
			} else {
				IFS::NameStat stat;
				while((err = fwfs->readdir(dir, stat)) >= 0) {
					copyfile(ffs, fwfs, stat);
				}
				fwfs->closedir(dir);
			}
		}
		delete fwfs;

		return ffs;
	}

	FileSystem* createFilesystem(SubType subtype)
	{
		switch(subtype) {
		case SubType::spiffs: {
			auto part = createSpiffsPartition(SPIFFS_IMGFILE, FFS_FLASH_SIZE);
			return IFS::createSpiffsFilesystem(part);
		}

		case SubType::littlefs: {
			auto part = createPartition(LFS_IMGFILE, FFS_FLASH_SIZE, F("lfs"), SubType::littlefs);
			return IFS::createLfsFilesystem(part);
		}

		default:
			return nullptr;
		}
	}

	FileSystem* initFWFS(Storage::Partition fwfsPart, SubType subtype)
	{
		FileSystem* fs;
		if(subtype == SubType::fwfs) {
			fs = IFS::createFirmwareFilesystem(fwfsPart);
		} else {
			auto ffs = createFilesystem(subtype);
			fs = ffs ? IFS::createHybridFilesystem(fwfsPart, ffs) : nullptr;
		}
		CHECK(fs != nullptr);
		if(fs == nullptr) {
			return nullptr;
		}

		int err = fs->mount();
		if(err < 0) {
			debug_e("Mount failed: %s", fs->getErrorString(err).c_str());
			delete fs;
			return nullptr;
		}

		return fs;
	}

	void readFileTest(FileSystem& fs, const String& filename, const IFS::Stat& stat)
	{
		Crypto::Md5 ctx;
		IFS::File file(&fs);
		if(!file.open(filename)) {
			debug_e("open('%s'): %s", filename.c_str(), file.getLastErrorString().c_str());
			TEST_ASSERT(false);
			return;
		}
		int res = file.readContent([&ctx](const char* buffer, size_t size) {
			ctx.update(buffer, size);
			return size;
		});
		if(res < 0) {
			debug_e("readContent('%s'): %s", filename.c_str(), file.getLastErrorString().c_str());
			TEST_ASSERT(false);
			return;
		}

		CHECK(res == int(stat.size));

		if(stat.size == 0) {
			return;
		}

		Crypto::Md5::Hash fileHash;

		// Fetch MD5 hash either from this volume or reference
		res = file.control(IFS::FCNTL_GET_MD5_HASH, fileHash.data(), fileHash.size());
		if(res == IFS::Error::NotFound || res == IFS::Error::NotSupported) {
			IFS::File ref(fwfsRef);
			CHECK(ref.open(filename));
			res = ref.control(IFS::FCNTL_GET_MD5_HASH, fileHash.data(), fileHash.size());
		}
		CHECK(res == int(fileHash.size()));

		auto hash = ctx.getHash();
		CHECK(hash == fileHash);
	}

	void writeThroughTest(FileSystem& fs, const String& filename, const IFS::Stat& stat)
	{
		IFS::FileSystem::Info info;
		CHECK(stat.fs->getinfo(info) >= 0);
		if(info.type != IFS::FileSystem::Type::FWFS) {
			return; // Already writeable
		}

		auto originalStat = stat;
		auto originalContent = fs.getContent(filename);

		// On the hybrid volume this will copy FW file onto SPIFFS
		IFS::FileSystem::Info fsinfo;
		fs.getinfo(fsinfo);
		if(filename.length() > fsinfo.maxPathLength || stat.name.length > fsinfo.maxNameLength) {
			debug_w("** Skipping: File name too long for writethrough");
			return;
		}

		IFS::File file(&fs);
		file.open(filename, IFS::OpenFlag::Write | IFS::OpenFlag::Append);
		if(stat.attr[IFS::FileAttribute::ReadOnly]) {
			debug_i("** Skipping copy, '%s' is marked read-only", stat.name.buffer);
			REQUIRE(!file);
			return;
		}

		if(!file) {
			debug_i("open('%s'): %s", stat.name.buffer, file.getLastErrorString().c_str());
			return;
		}

		file.write(nullptr, 0);
		file.close();

		file.open(filename, IFS::File::ReadOnly);
		CHECK(file);
		IFS::Stat copyStat;
		CHECK(file.stat(copyStat));
		file.close();

		printFileInfo(Serial, copyStat);

		CHECK(copyStat.size == originalStat.size);
		CHECK(copyStat.attr == originalStat.attr);
		CHECK(copyStat.compression == originalStat.compression);
		auto now = IFS::fsGetTimeUTC();
		CHECK(abs(copyStat.mtime - now) < 3);

		if(copyStat.acl != originalStat.acl) {
			/*
			 * Don't throw an error here. FWFS behaviour is to inherit the root directory ACL,
			 * so there may not be
			 */
			debug_w("Warning: ACL mismatch");

			/*
			 * ACLs are optional attributes on files. If not provided, they should default to the
			 * root directory ACL. SPIFFS doesn't have full directory support (it's currently emulated)
			 * so this isn't current possible to handle. But for other filesystems (LittleFS) confirm this.
			 */
			IFS::FileSystem::Info fsinfo;
			copyStat.fs->getinfo(fsinfo);
			if(fsinfo.type != IFS::FileSystem::Type::SPIFFS) {
				TEST_ASSERT(false);
			}
		}

		auto copyContent = fs.getContent(filename);
		REQUIRE(copyContent == originalContent);
	}

	int scandir(FileSystem& fs, const String& path, Flags flags)
	{
		debug_i("Scanning '%s'", path.c_str());

		Vector<String> files;
		Vector<String> directories;
		{
			IFS::Directory dir(&fs);
			dir.open(path);
			while(dir.next()) {
				auto name = dir.stat().name.buffer;
				if(dir.stat().isDir()) {
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

			IFS::Stat stat;
			int err = fs.stat(filename, &stat);
			if(err < 0) {
				debug_e("> %s: %d", filename.c_str(), err);
				continue;
			}
			stat.name = filename;
			printFileInfo(Serial, stat);

			if(flags[Flag::readFileTest]) {
				readFileTest(fs, filename, stat);
			}

			if(flags[Flag::writeThroughTest]) {
				writeThroughTest(fs, filename, stat);
			}
		}

		for(auto& name : directories) {
			String subdir = path;
			if(path.length() != 0) {
				subdir += '/';
			}
			subdir += name;
			scandir(fs, subdir, flags);
		}

		return FS_OK;
	}

	void fstest(FileSystem& fs, Flags flags)
	{
		Serial.println();
		Serial.print("FS Test: ");
		Serial.println(toString(flags));

		printFsInfo(Serial, fs);

		int res = fs.check();
		debug_i("check(): %s", fs.getErrorString(res).c_str());
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
