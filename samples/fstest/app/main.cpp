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

#include <stdio.h>
#include <assert.h>
#include "except.h"
#include "flashmem.h"
#include "IFS/HybridFileSystem.h"
#include "../../ifs-host/StdFileMedia.h"
#include "IFS/IFSFlashMedia.h"
#include "IFS/FWObjectStore.h"
#include "IFS/SPIFFSObjectStore.h"

#include <string>
using String = std::string;

// We mount SPIFFS on a file so we can inspect if necessary
const char* FLASHMEM_DMP = "flashmem.dmp";

// Global filesystem instance
static IFileSystem* g_filesys;

//
#ifdef __WIN32

#define INCBIN(name, file)                                                                                             \
	__asm__(".section .rodata\n"                                                                                       \
			".global _" #name "\n"                                                                                     \
			".def _" #name "; .scl 2; .type 32; .endef\n"                                                              \
			".align 4\n"                                                                                               \
			"_" #name ":\n"                                                                                            \
			".incbin \"" file "\"\n"                                                                                   \
			".global _" #name "_end\n"                                                                                 \
			".def _" #name "_end; .scl 2; .type 32; .endef\n"                                                          \
			".align 4\n"                                                                                               \
			"_" #name "_end:\n"                                                                                        \
			".byte 0\n");                                                                                              \
	extern const __attribute__((aligned(4))) uint8_t name[];                                                           \
	extern const __attribute__((aligned(4))) uint8_t name##_end[];                                                     \
	const uint32_t name##_len = name##_end - name;

#else

#define INCBIN(name, file)                                                                                             \
	__asm__(".section .rodata\n"                                                                                       \
			".global " #name "\n"                                                                                      \
			".type " #name ", @object\n"                                                                               \
			".align 4\n" #name ":\n"                                                                                   \
			".incbin \"" file "\"\n" #name "_end:\n");                                                                 \
	extern const __attribute__((aligned(4))) uint8_t name[];                                                           \
	extern const __attribute__((aligned(4))) uint8_t name##_end[];                                                     \
	const uint32_t name##_len = name##_end - name;

#endif

INCBIN(_fwfiles_data, "out/fwfiles.bin")
//INCBIN(_fwfiles_data1, "out/fwfiles1.bin")

// Flash Filesystem parameters
#define FFS_SECTOR_COUNT 128
#define FFS_FLASH_SIZE (FFS_SECTOR_COUNT * INTERNAL_FLASH_SECTOR_SIZE)

// Arbitrary limit on file size
#define FWFILE_MAX_SIZE 0x100000

const char* getErrorText(IFileSystem* fs, int err)
{
	static char msg[64];
	fs->geterrortext(err, msg, sizeof(msg));
	return msg;
}

const char* getErrorText(int err)
{
	return getErrorText(g_filesys, err);
}

int copyfile(IFileSystem& dst, IFileSystem& src, const FileStat& stat)
{
	if(bitRead(stat.attr, FileAttr::Directory)) {
		return FSERR_NotSupported;
	}

	int res = FS_OK;

	file_t srcfile = src.fopen(stat, eFO_ReadOnly);
	if(srcfile < 0) {
		res = srcfile;
	} else {
		file_t dstfile = dst.open(stat.name, eFO_CreateNewAlways);
		if(dstfile < 0) {
			res = dstfile;
		} else {
			uint8_t buffer[512];
			for(;;) {
				res = src.read(srcfile, buffer, sizeof(buffer));
				if(res <= 0) {
					break;
				}
				int nread = res;

				res = dst.write(dstfile, buffer, nread);
				if(res < 0) {
					break;
				}
			}
			dst.close(dstfile);
		}
		src.close(srcfile);
	}

	debugf("Copy '%s': %s", stat.name.buffer, getErrorText(res));

	return res;
}

/* Copy some data into SPIFFS */
void initSpiffs()
{
	// Mount source data image
	//	flashmem_write(_fwfiles_data1, 0, _fwfiles_data1_len);
	auto fwMedia1 = new IFSFlashMedia(_fwfiles_data_len, eFMA_ReadOnly);
	auto fwStore1 = new FWObjectStore(fwMedia1);
	auto hfs = new FirmwareFileSystem(fwStore1);

	int res = hfs->mount();
	if(res >= 0) {
		// Mount SPIFFS
		auto ffsMedia = new StdFileMedia(FLASHMEM_DMP, FFS_FLASH_SIZE, INTERNAL_FLASH_SECTOR_SIZE, eFMA_ReadWrite);
		auto spf = new SPIFlashFileSystem(ffsMedia);

		res = spf->mount();
		if(res >= 0) {
			filedir_t dir;
			res = hfs->opendir(nullptr, &dir);
			if(res >= 0) {
				FileNameStat stat;
				while((res = hfs->readdir(dir, &stat))) {
					copyfile(*spf, *hfs, stat);
				}
				hfs->closedir(dir);
			}
		}

		delete spf;
	}

	delete hfs;
}

/** @brief initialise filesystem
 *  @param imgfile optional name of FWFS image to load
 *  @retval bool true on success
 */
bool fsInit(const char* imgfile)
{
	//	initSpiffs();

	debugf("fwfiles_data = 0x%08X, end = 0x%08X, len = 0x%08X", uint32_t(_fwfiles_data), uint32_t(_fwfiles_data_end),
		   _fwfiles_data_len);

	IFSMedia* fwMedia;
	if(imgfile != nullptr) {
		fwMedia = new StdFileMedia(imgfile, FWFILE_MAX_SIZE, INTERNAL_FLASH_SECTOR_SIZE, eFMA_ReadOnly);
	} else {
		flashmem_write(_fwfiles_data, 0, _fwfiles_data_len);
		fwMedia = new IFSFlashMedia(uint32_t(0), eFMA_ReadOnly);
	}

	auto store = new FWObjectStore(fwMedia);

#ifdef HYBRID_TEST

	IFSMedia* ffsMedia = new StdFileMedia(FLASHMEM_DMP, FFS_FLASH_SIZE, INTERNAL_FLASH_SECTOR_SIZE, eFMA_ReadWrite);
	auto hfs = new HybridFileSystem(store, ffsMedia);

#else

	auto hfs = new FirmwareFileSystem(store);

	/*
	flashmem_write(_fwfiles_data1, _fwfiles_data_len, _fwfiles_data1_len);
	auto fwMedia1 = new IFSFlashMedia(_fwfiles_data_len, eFMA_ReadOnly);
	auto store1 = new FWObjectStore(fwMedia1);
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
		return false;
	}

	g_filesys = hfs;
	return true;
}

void fsinfo()
{
	FileSystemInfo info;
	int res = g_filesys->getinfo(info);
	if(res < 0) {
		debug_e("fileSystemGetInfo(): %s", getErrorText(res));
		return;
	}

	char typeStr[10];
	debugf("type:       %s", fileSystemTypeToStr(info.type, typeStr, sizeof(typeStr)));
	if(info.media) {
		debugf("mediaType:  %u", info.media->type());
		debugf("bus:        %u", info.media->bus());
	}
	debugf("attr:       0x%02X", info.attr);
	debugf("volumeID:   0x%08X", info.volumeID);
	debugf("name:       %s", info.name.buffer);
	debugf("volumeSize: %u", info.volumeSize);
	debugf("freeSpace:  %u", info.freeSpace);
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
	int result = true;

#ifdef READ_FILE_TEST

	file_t file = g_filesys->fopen(stat, eFO_ReadOnly);

	if(file < 0) {
		debug_w("fopen(): %s", getErrorText(file));
		return false;
	}

	{
		FileStat stat2;
		int res = g_filesys->fstat(file, &stat2);
		if(res < 0) {
			debug_w("fstat(): %s", getErrorText(res));
		}
	}

	char buf[1024];
	uint32_t total = 0;
	while(!g_filesys->eof(file)) {
		int len = g_filesys->read(file, buf, sizeof(buf));
		if(len <= 0) {
			if(len < 0) {
				debug_e("Error! %s", getErrorText(len));
				result = false;
			}
			break;
		}
		//			if(total == 0)
		//			m_printHex("data", buf, len); //std::min(len, 32));
		total += len;
	}
	g_filesys->close(file);

	if(total != stat.size) {
		debug_e("Size mismatch: stat reports %u bytes, read %u bytes", stat.size, total);
		result = false;
	}
#endif

	return result;
}

int scandir(const String& path)
{
	debugf("Scanning '%s'", path.c_str());

	filedir_t dir;
	int res = g_filesys->opendir(path.c_str(), &dir);
	if(res < 0) {
		return res;
	}

	FileNameStat stat;
	while((res = g_filesys->readdir(dir, &stat)) >= 0) {
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
		debugf("%-50s %6u %s #0x%04x %s %s %s %s", (const char*)stat.name, stat.size, typestr, stat.id, aclstr, attrstr,
			   cmpstr, timestr);

		readFileTest(stat);

		if(bitRead(stat.attr, FileAttr::Directory)) {
			if(path.empty()) {
				scandir(stat.name.buffer);
			} else {
				scandir(path + '/' + stat.name.buffer);
			}
			continue;
		}

#ifdef WRITE_THROUGH_TEST

		if(bitRead(stat.attr, FileAttr::ReadOnly)) {
			continue;
		}

		{
			// On the hybrid volume this will copy FW file onto SPIFFS
			file_t file = g_filesys->open(stat, eFO_WriteOnly | eFO_Append);
			if(file < 0)
				debug_e("open('%s') failed, %s", stat.name.buffer, fileGetErrorString(file).c_str());
			else {
				g_filesys->write(file, nullptr, 0);
				g_filesys->close(file);
			}
		}
#endif
	}

	debugf("readdir('%s'): %s", path.c_str(), getErrorText(res));

	g_filesys->closedir(dir);

	return FS_OK;
}

void fstest()
{
	int res = g_filesys->check();
	debugf("check(): %s", getErrorText(res));

	//  fs.format();

	//  auto f = fs.open("favicon.ico", eFO_CreateNewAlways | eFO_WriteOnly);
	//  delete f;

	//  auto res = fs.deleteFile(".auth - Copy.json");
	//  auto res = fs.remove("askfhsdfk");
	//  debugf("remove: %d", res);

	/*
	FileStat stat;
	int res = fileStats("iocontrol.js", &stat);
	debugf("stat(): %s [%d]", fileGetErrorString(res).c_str(), res);
	FileStream stream(stat);
	debugf("stream.isValid(): %d", stream.isValid());
	char buffer[2048];
	auto len = stream.readMemoryBlock(buffer, sizeof(buffer));
	debugf("fs.read(%u): %d", sizeof(buffer), len);
	if (len > 0)
		m_printHex("Content", buffer, len);
	stream.close();

	res = fileStats("system.js", &stat);
	debugf("stat(): %s [%d]", fileGetErrorString(res).c_str(), res);
*/

	scandir("");
}

int main(int argc, char* argv[])
{
	trap_exceptions();

	const char* imgfile = (argc > 1) ? argv[1] : nullptr;

	if(!fsInit(imgfile)) {
		debug_e("fs init failed");
		return 1;
	}

	fsinfo();

	fstest();

	delete g_filesys;
}
