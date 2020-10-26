/*
 * main.cpp
 *
 *  Created on: 22 Jul 2018
 *      Author: mikee47
 *
 * For testing file systems
 */

// Mount hybrid FWFS/SPIFFS filesystem; if not defined, uses FWFS only
//#define HYBRID_TEST

// Read content of every file
#define READ_FILE_TEST

// Hybrid filesystem, 'touch' files to propagate them to SPIFFS from FWFS
//#define WRITE_THROUGH_TEST

#include <Platform/System.h>
#include <WString.h>
#include <assert.h>
#include <esp_spi_flash.h>
#include <IFS/HYFS/FileSystem.h>
#include <IFS/StdFileMedia.h>
#include <IFS/MemoryMedia.h>
#include <IFS/FlashMedia.h>
#include <IFS/FWFS/ObjectStore.h>
#include <IFS/Helpers.h>
#include <HardwareSerial.h>

using namespace IFS;

namespace
{
// We mount SPIFFS on a file so we can inspect if necessary
const char* FLASHMEM_DMP = "out/flashmem.dmp";

IMPORT_FSTR(fwfsImage1, PROJECT_DIR "/out/fwfsImage1.bin")
//IMPORT_FSTR(fwfsImage2, PROJECT_DIR "/out/fwfsImage2.bin")

// Flash Filesystem parameters
#define FFS_SECTOR_COUNT 128
#define FFS_FLASH_SIZE (FFS_SECTOR_COUNT * INTERNAL_FLASH_SECTOR_SIZE)

// Arbitrary limit on file size
#define FWFILE_MAX_SIZE 0x100000

const char* getErrorText(IFileSystem* fs, int err)
{
	static char msg[64];
	if(fs == nullptr) {
		IFS::fsGetErrorText(err, msg, sizeof(msg));
	} else {
		fs->geterrortext(err, msg, sizeof(msg));
	}
	return msg;
}

int copyfile(IFileSystem* dst, IFileSystem* src, const FileStat& stat)
{
	if(bitRead(stat.attr, FileAttr::Directory)) {
		return FSERR_NotSupported;
	}

	int res = FS_OK;
	IFileSystem* fserr{};

	file_t srcfile = src->fopen(stat, FileOpenFlag::Read);
	if(srcfile < 0) {
		res = srcfile;
		fserr = src;
	} else {
		file_t dstfile = dst->open(stat.name, FileOpenFlag::Create | FileOpenFlag::Write);
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

	debug_i("Copy '%s': %s", stat.name.buffer, getErrorText(fserr, res));

	return res;
}

IFileSystem* initSpiffs()
{
	// Mount SPIFFS
	auto ffsMedia = new StdFileMedia(FLASHMEM_DMP, FFS_FLASH_SIZE, INTERNAL_FLASH_SECTOR_SIZE, Media::ReadWrite);
	auto ffs = new SPIFFS::FileSystem(ffsMedia);

	int err = ffs->mount();
	if(err < 0) {
		debug_e("Mount failed: %s", getErrorText(ffs, err));
		delete ffs;
		return nullptr;
	}

	FileSystemInfo info;
	err = ffs->getinfo(info);
	if(err < 0) {
		debug_e("Failed to get FS info: %s", getErrorText(ffs, err));
		delete ffs;
		return nullptr;
	}

	if(info.freeSpace < info.volumeSize) {
		// Filesystem is populated
		return ffs;
	}

	debug_i("SPIFFS is empty, copy some stuff from FWFS");

	// Mount source data image
	auto fwfs = createFirmwareFilesystem(fwfsImage1.data());
	int res = fwfs->mount();
	if(res < 0) {
		debug_e("FWFS mount failed: %s", getErrorText(fwfs, err));
	} else {
		filedir_t dir;
		err = fwfs->opendir(nullptr, &dir);
		if(err < 0) {
			debug_e("FWFS opendir failed: %s", getErrorText(fwfs, err));
		} else {
			FileNameStat stat;
			while((err = fwfs->readdir(dir, &stat)) >= 0) {
				copyfile(ffs, fwfs, stat);
			}
			fwfs->closedir(dir);
		}
	}
	delete fwfs;

	return ffs;
}

/** @brief initialise filesystem
 *  @param imgfile optional name of FWFS image to load
 *  @retval bool true on success
 */
IFileSystem* initFWFS(const char* imgfile)
{
	debug_i("fwfiles_data = %p, len = 0x%08X", fwfsImage1.data(), fwfsImage1.length());

	IFS::Media* fwMedia;
	if(imgfile != nullptr) {
		fwMedia = new StdFileMedia(imgfile, FWFILE_MAX_SIZE, INTERNAL_FLASH_SECTOR_SIZE, Media::ReadOnly);
	} else {
		flashmem_write(fwfsImage1.data(), 0, fwfsImage1.size());
		fwMedia = new FlashMedia(uint32_t(0), Media::ReadOnly);
	}

	auto store = new FWFS::ObjectStore(fwMedia);

#ifdef HYBRID_TEST

	auto ffsMedia = new StdFileMedia(FLASHMEM_DMP, FFS_FLASH_SIZE, INTERNAL_FLASH_SECTOR_SIZE, Media::ReadWrite);
	auto hfs = new HybridFileSystem(store, ffsMedia);

#else

	auto hfs = new FWFS::FileSystem(store);

	/*
	flashmem_write(_fwfiles_data1, _fwfiles_data_len, _fwfiles_data1_len);
	auto fwMedia1 = new IFSFlashMedia(_fwfiles_data_len, eFMA_ReadOnly);
	auto store1 = new FWFS::ObjectStore(fwMedia1);
*/

	/*
 * Test SPIFFS object store
		auto ffsMedia = new StdFileMedia(FLASHMEM_DMP, FFS_FLASH_SIZE, FLASH_SECTOR_SIZE, eFMA_ReadWrite);
		auto store1 = new SPIFFSObjectStore(ffsMedia);
		hfs->setVolume(1, store1);
*/

#endif

	int err = hfs->mount();
	if(err < 0) {
		debug_e("Mount failed: %s", getErrorText(hfs, err));
		delete hfs;
		hfs = nullptr;
	}

	return hfs;
}

void printFsInfo(IFileSystem* fs)
{
	char namebuf[256];
	FileSystemInfo info(namebuf, sizeof(namebuf));
	int res = fs->getinfo(info);
	if(res < 0) {
		debug_e("fileSystemGetInfo(): %s", getErrorText(fs, res));
		return;
	}

	char typeStr[10];
	debug_i("type:       %s", fileSystemTypeToStr(info.type, typeStr, sizeof(typeStr)));
	if(info.media) {
		debug_i("mediaType:  %u", info.media->type());
		debug_i("bus:        %u", info.media->bus());
	}
	debug_i("attr:       0x%02X", info.attr);
	debug_i("volumeID:   0x%08X", info.volumeID);
	debug_i("name:       %s", info.name.buffer);
	debug_i("volumeSize: %u", info.volumeSize);
	debug_i("freeSpace:  %u", info.freeSpace);
}

// Displays as local time
const char* timeToStr(char* buffer, time_t t, const char* dtsep)
{
	struct tm* tm = localtime(&t);
	sprintf(buffer, "%02d/%02d/%04d%s%02d:%02d:%02d", tm->tm_mday, tm->tm_mon + 1, 1900 + tm->tm_year, dtsep,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	return buffer;
}

bool readFileTest(const FileStat& stat)
{
	int result{true};

#ifdef READ_FILE_TEST

	auto fs{stat.fs};

	file_t file = fs->fopen(stat, FileOpenFlag::Read);

	if(file < 0) {
		debug_w("fopen(): %s", getErrorText(fs, file));
		return false;
	}

	{
		FileStat stat2;
		int res = fs->fstat(file, &stat2);
		if(res < 0) {
			debug_w("fstat(): %s", getErrorText(fs, res));
		}
	}

	char buf[1024];
	uint32_t total{0};
	while(!fs->eof(file)) {
		int len = fs->read(file, buf, sizeof(buf));
		if(len <= 0) {
			if(len < 0) {
				debug_e("Error! %s", getErrorText(fs, len));
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
#endif

	return result;
}

int scandir(IFileSystem* fs, const String& path)
{
	debug_i("Scanning '%s'", path.c_str());

	filedir_t dir;
	int res = fs->opendir(path.c_str(), &dir);
	if(res < 0) {
		return res;
	}

	FileNameStat stat;
	while((res = fs->readdir(dir, &stat)) >= 0) {
		FileSystemInfo info;
		stat.fs->getinfo(info);
		char typestr[5];
		fileSystemTypeToStr(info.type, typestr, sizeof(typestr));
		char aclstr[5];
		fileAclToStr(stat.acl, aclstr, sizeof(aclstr));
		char attrstr[10];
		fileAttrToStr(stat.attr, attrstr, sizeof(attrstr));
		char cmpstr[10];
		compressionTypeToStr(stat.compression, cmpstr, sizeof(cmpstr));
		char timestr[20];
		timeToStr(timestr, stat.mtime, " ");
		debug_i("%-50s %6u %s #0x%04x %s %s %s %s", (const char*)stat.name, stat.size, typestr, stat.id, aclstr,
				attrstr, cmpstr, timestr);

		readFileTest(stat);

		if(bitRead(stat.attr, FileAttr::Directory)) {
			if(path.length() == 0) {
				scandir(fs, stat.name.buffer);
			} else {
				scandir(fs, path + '/' + stat.name.buffer);
			}
			continue;
		}

#ifdef WRITE_THROUGH_TEST

		if(bitRead(stat.attr, FileAttr::ReadOnly)) {
			continue;
		}

		{
			// On the hybrid volume this will copy FW file onto SPIFFS
			file_t file = fs->open(stat, FileOpenFlag::Write | FileOpenFlag::Append);
			if(file < 0)
				debug_e("open('%s') failed, %s", stat.name.buffer, fileGetErrorString(file).c_str());
			else {
				fs->write(file, nullptr, 0);
				fs->close(file);
			}
		}
#endif
	}

	debug_i("readdir('%s'): %s", path.c_str(), getErrorText(fs, res));

	fs->closedir(dir);

	return FS_OK;
}

void fstest(IFileSystem* fs)
{
	int res = fs->check();
	debug_i("check(): %s", getErrorText(fs, res));

	//  fs.format();

	//  auto f = fs.open("favicon.ico", eFO_CreateNewAlways | eFO_WriteOnly);
	//  delete f;

	//  auto res = fs.deleteFile(".auth - Copy.json");
	//  auto res = fs.remove("askfhsdfk");
	//  debug_i("remove: %d", res);

	/*
	FileStat stat;
	int res = fileStats("iocontrol.js", &stat);
	debug_i("stat(): %s [%d]", fileGetErrorString(res).c_str(), res);
	FileStream stream(stat);
	debug_i("stream.isValid(): %d", stream.isValid());
	char buffer[2048];
	auto len = stream.readMemoryBlock(buffer, sizeof(buffer));
	debug_i("fs.read(%u): %d", sizeof(buffer), len);
	if (len > 0)
		m_printHex("Content", buffer, len);
	stream.close();

	res = fileStats("system.js", &stat);
	debug_i("stat(): %s [%d]", fileGetErrorString(res).c_str(), res);
*/

	scandir(fs, "");
}

} // namespace

void init()
{
	Serial.begin();
	Serial.systemDebugOutput(true);

	//	const char* imgfile = (argc > 1) ? argv[1] : nullptr;
	char* imgfile{nullptr};

	Serial.println();
	auto fs = initFWFS(imgfile);
	if(fs != nullptr) {
		printFsInfo(fs);
		fstest(fs);
		delete fs;
	}

	Serial.println();
	fs = initSpiffs();
	if(fs != nullptr) {
		printFsInfo(fs);
		fstest(fs);
		delete fs;
	}

	System.restart();
}
