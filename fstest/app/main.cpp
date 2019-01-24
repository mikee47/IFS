/*
 * main.cpp
 *
 *  Created on: 22 Jul 2018
 *      Author: mikee47
 *
 * For testing file systems
 */

// Mount hybrid FWFS/SPIFFS filesystem; if not defined, uses FWFS only
#define HYBRID_TEST

// Read content of every file
//#define READ_FILE_TEST

// Hybrid filesystem, 'touch' files to propagate them to SPIFFS from FWFS
//#define WRITE_THROUGH_TEST

#include <stdio.h>
#include <assert.h>
#include "Data/Stream/TemplateFileStream.h"
#include "Data/CircularBuffer.h"
#include "DateTime.h"
#include "FileSystem.h"
#include "Services/IFS/HybridFileSystem.h"
#include "Services/IFS/StdFileMedia.h"
#include "Services/IFS/IFSFlashMedia.h"
#include "Services/IFS/FWObjectStore.h"
#include "Services/IFS/SPIFFSObjectStore.h"
#include <except.h>
#include "Network/Http/HttpCommon.h"
#include "flashmem.h"
#include "JsonConfigFile.h"

#define LOG_PAGE_SIZE 256

const char* FLASHMEM_DMP = "flashmem.dmp";

// @todo bung this somewhere handy
// @todo perhaps declare this using an appropriate struct containing (ptr, length)
#define INCBIN(_name, _file)                                                                                           \
	__asm__(".section .irom.text\n"                                                                                    \
			".global _" #_name "\n"                                                                                    \
			".def _" #_name "; .scl 2; .type 32; .endef\n"                                                             \
			".align 4\n"                                                                                               \
			"_" #_name ":\n"                                                                                           \
			".incbin \"" _file "\"\n"                                                                                  \
			".global _" #_name "_end\n"                                                                                \
			".def _" #_name "_end; .scl 2; .type 32; .endef\n"                                                         \
			".align 4\n"                                                                                               \
			"_" #_name "_end:\n"                                                                                       \
			".byte 0\n");                                                                                              \
	extern const __attribute__((aligned(4))) uint8_t _name[];                                                          \
	extern const __attribute__((aligned(4))) uint8_t _name##_end[];                                                    \
	const uint32_t _name##_len = _name##_end - _name;

INCBIN(_fwfiles_data, "../../lightcon/out/fwfiles.bin")
INCBIN(_fwfiles_data1, "../../lightcon/out/fwfiles1.bin")

#define FFS_SECTOR_COUNT 128
#define FFS_FLASH_SIZE (FFS_SECTOR_COUNT * INTERNAL_FLASH_SECTOR_SIZE)

// Arbitrary limit on file size
#define FWFILE_MAX_SIZE 0x100000

static size_t __nputs(const char* str, size_t len)
{
	return fwrite(str, 1, len, stdout);
}



int copyfile(IFileSystem& dst, IFileSystem& src, const FileStat& stat)
{
	if (bitRead(stat.attr, FileAttr::Directory))
		return FSERR_NotSupported;

	int res = FS_OK;

	file_t srcfile = src.fopen(stat, eFO_ReadOnly);
	if (srcfile < 0)
		res = srcfile;
	else {
		file_t dstfile = dst.open(stat.name, eFO_CreateNewAlways);
		if (dstfile < 0)
			res = dstfile;
		else {
			uint8_t buffer[512];
			for(;;) {
				res = src.read(srcfile, buffer, sizeof(buffer));
				if(res <= 0)
					break;
				int nread = res;

				res = dst.write(dstfile, buffer, nread);
				if(res < 0)
					break;
			}
			dst.close(dstfile);
		}
		src.close(srcfile);
	}

	char msg[64];
	debug_i("Copy '%s': %s", stat.name.buffer, fileGetErrorText(res, msg, sizeof(msg)));

	return res;
}


/* Copy some data into SPIFFS */
void initSpiffs()
{
	// Mount source data image
	flashmem_write_internal(_fwfiles_data1, 0, _fwfiles_data1_len);
	auto fwMedia1 = new IFSFlashMedia(_fwfiles_data_len, eFMA_ReadOnly);
	auto fwStore1 = new FWObjectStore(fwMedia1);
	auto hfs = new FirmwareFileSystem(fwStore1);

	int res = hfs->mount();
	if (res >= 0) {
		// Mount SPIFFS
		auto ffsMedia = new StdFileMedia(FLASHMEM_DMP, FFS_FLASH_SIZE, INTERNAL_FLASH_SECTOR_SIZE, eFMA_ReadWrite);
		auto spf = new SPIFlashFileSystem(ffsMedia);

		res = spf->mount();
		if (res >= 0) {
			filedir_t dir;
			res = hfs->opendir(nullptr, &dir);
			if (res >= 0) {
				FileNameStat stat;
				while ((res = hfs->readdir(dir, &stat)))
					copyfile(*spf, *hfs, stat);
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

	debug_i("fwfiles_data = 0x%08X, end = 0x%08X, len = 0x%08X", _fwfiles_data, _fwfiles_data_end, _fwfiles_data_len);

	IFSMedia* fwMedia;
	if(imgfile)
		fwMedia = new StdFileMedia(imgfile, FWFILE_MAX_SIZE, INTERNAL_FLASH_SECTOR_SIZE, eFMA_ReadOnly);
	else {
		flashmem_write_internal(_fwfiles_data, 0, _fwfiles_data_len);
		fwMedia = new IFSFlashMedia((uint32_t)0, eFMA_ReadOnly);
	}

	auto store = new FWObjectStore(fwMedia);

#ifdef HYBRID_TEST

	IFSMedia* ffsMedia = new StdFileMedia(FLASHMEM_DMP, FFS_FLASH_SIZE, INTERNAL_FLASH_SECTOR_SIZE, eFMA_ReadWrite);
	auto hfs = new HybridFileSystem(store, ffsMedia);

#else

	auto hfs = new FirmwareFileSystem(store);

/*
	flashmem_write_internal(_fwfiles_data1, _fwfiles_data_len, _fwfiles_data1_len);
	auto fwMedia1 = new IFSFlashMedia(_fwfiles_data_len, eFMA_ReadOnly);
	auto store1 = new FWObjectStore(fwMedia1);
*/

//	auto ffsMedia = new StdFileMedia(FLASHMEM_DMP, FFS_FLASH_SIZE, INTERNAL_FLASH_SECTOR_SIZE, eFMA_ReadWrite);
//	auto store1 = new SPIFFSObjectStore(ffsMedia);
//	hfs->setVolume(1, store1);

#endif

	int err = hfs->mount();
	if(err < 0) {
		char buf[64];
		hfs->geterrortext(err, buf, sizeof(buf));
		debug_e("Mount failed: %s", buf);
		delete hfs;
		return false;
	}

	fileSetFileSystem(hfs);
	return true;
}

void fsinfo()
{
	FileSystemInfo info;
	int res = fileGetSystemInfo(info);
	if(res < 0) {
		debug_e("fileSystemGetInfo(): %s", fileGetErrorString(res).c_str());
		return;
	}

	debug_i("type:       %u", info.type);
	if(info.media) {
		debug_i("mediaType:  %u", info.media->type());
		debug_i("bus:        %u", info.media->bus());
	}
	debug_i("attr:       0x%02X", info.attr);
	debug_i("volumeID:   0x%08X", info.volumeID);
	debug_i("name:       %s", info.name);
	debug_i("volumeSize: %u", info.volumeSize);
	debug_i("freeSpace:  %u", info.freeSpace);
}

int scandir(const String& path)
{
	debug_i("Scanning '%s'", path.c_str());

	filedir_t dir;
	int res = fileOpenDir(path.c_str(), &dir);
	if(res < 0)
		return res;

	FileNameStat stat;
	while((res = fileReadDir(dir, &stat)) >= 0) {
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
		debug_i("%-50s %6u %s #0x%04x %s %s %s %s", (const char*)stat.name, stat.size, typestr, stat.id, aclstr, attrstr,
				cmpstr, DateTime(stat.mtime).toFullDateTimeString().c_str());

#ifdef READ_FILE_TEST

		file_t file = fileOpen(stat);

		if(file < 0) {
			debug_w("fileOpen(): %s", fileGetErrorString(file).c_str());
			continue;
		}

		{
			FileStat stat2;
			int res = fileStats(file, &stat2);
			if(res < 0)
				debug_w("fileStats(): %s", fileGetErrorString(res).c_str());
		}

		char buf[1024];
		uint32_t total = 0;
		while(!fileIsEOF(file)) {
			int len = fileRead(file, buf, sizeof(buf));
			if(len <= 0) {
				if (len < 0)
					debug_e("Error! %s", fileGetErrorString(len).c_str());
				break;
			}
//			if(total == 0)
				m_printHex("data", buf, len);	//std::min(len, 32));
			total += len;
		}
		fileClose(file);

		if(total != stat.size)
			debug_e("Size mismatch: stat reports %u bytes, read %u bytes", stat.size, total);
#endif

		if(bitRead(stat.attr, FileAttr::Directory)) {
			if(path)
				scandir(path + "/" + stat.name);
			else
				scandir(stat.name.buffer);
			continue;
		}

#ifdef WRITE_THROUGH_TEST

		if(bitRead(stat.attr, FileAttr::ReadOnly))
			continue;

		{
			// On the hybrid volume this will copy FW file onto SPIFFS
			file_t file = fileOpen(stat, eFO_WriteOnly | eFO_Append);
			if(file < 0)
				debug_e("fileOpen('%s') failed, %s", stat.name.buffer, fileGetErrorString(file).c_str());
			else {
				fileTouch(file);
				fileClose(file);
			}
		}
#endif

	}

	debug_i("fileReadDir('%s'): %s", path.c_str(), fileGetErrorString(res).c_str());

	fileCloseDir(dir);

	return FS_OK;
}

void fstest()
{
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


	scandir(nullptr);
}

void bufTest()
{
	file_t file = fileOpen("apple-touch-icon-180x180.png");
	if(file < 0)
		return;

	uint8_t data[128000];
	int datalen = fileRead(file, data, sizeof(data));
	fileClose(file);

	debug_i("datalen = %d", datalen);

	CircularBuffer buf(256);

	FILE* fp = fopen("test.png", "wb");
	debug_i("fp: %s", fp ? "OK" : "ERR");

	int offset = 0;
	do {
		int len = std::min(datalen - offset, 33);
		int written = buf.write(data + offset, len);
		offset += written;

		if(written == 0) {
			char tmp[1024];
			len = buf.readMemoryBlock(tmp, sizeof(tmp));
			buf.seek(len);
			fwrite(tmp, 1, len, fp);
		}
	} while(offset < datalen || buf.available());

	fclose(fp);
}

void templateStreamCheck()
{
	const auto status = HTTP_STATUS_FORBIDDEN;

	auto tmpl = new TemplateFileStream("error.html");
	auto& vars = tmpl->variables();
	vars["path"] = "/system.html";
	vars["code"] = status;
	vars["text"] = httpGetStatusText(status);

	int len;
	char buffer[4096];
	unsigned pos = 0;
	for(;;) {
		len = tmpl->readMemoryBlock(&buffer[pos], sizeof(buffer) - pos);
		debug_i("readMemoryBlock() returned %d", len);
		if(len <= 0)
			break;
		pos += len;
		tmpl->seek(len);
	}
	while(len > 0)
		;

	m_nputs(buffer, pos);
	m_putc('\n');

	//	debug_hex(DBG, "OUTPUT: ", buffer, pos);

	delete tmpl;
}


#include "v4test/v4test.h"

class StdPrint: public Print
{
protected:
	size_t write(uint8_t c) override
	{
		return m_putc(c);
	}

	size_t write(const uint8_t* buffer, size_t size) override
	{
		return m_nputs((const char*)buffer, size);
	}

};



StdPrint stdprint;

int main(int argc, char* argv[])
{
	trap_exceptions();

	m_setPuts(__nputs);

/*
	m_printHex("TEST1", (void*)main, 130);
	m_printHex("TEST2", (void*)main, 130, 0);
	m_printHex("TEST3", (void*)main, 130, 0xFFFFF9, 8);
	m_printHex(nullptr, (void*)main, 130, 0xFFFFF9, 0);
*/
	testAll(stdprint);

	return 0;

/*
	DynamicJsonBuffer buffer;
	auto& json = buffer.createObject();
	char buf[10];
	json["access"] = String(userRoleToStr(UserRole::admin, buf, sizeof(buf)));
*/

	const char* imgfile = (argc > 1) ? argv[1] : nullptr;

	if(!fsInit(imgfile)) {
		debug_e("fs init failed");
		return 1;
	}

	//	fileSystemCheck();

	//	fsinfo();

	fstest();

	//	templateStreamCheck();

	//	bufTest();

	fileFreeFileSystem();
}
