/*
 * Extents.cpp
 *
 *  Created on: 23 March 2023
 *      Author: mikee47
 */

#include <FsTest.h>
#include <Spiffs.h>
#include <LittleFS.h>

namespace
{
class ExtentStream
{
public:
	ExtentStream(Storage::Partition part, IFS::Extent* list, uint16_t extcount)
		: part(part), ext(list[0]), list(list), extCount(extcount)
	{
	}

	size_t read(void* buffer, size_t count)
	{
		auto bufptr = static_cast<uint8_t*>(buffer);
		size_t res{0};
		while(res < count && extIndex < extCount) {
			size_t len = std::min(ext.length, uint32_t(count - res));
			part.read(ext.offset, bufptr, len);
			bufptr += len;
			res += len;
			ext.offset += len;
			ext.length -= len;
			if(ext.length) {
				continue;
			}
			if(ext.repeat--) {
				ext.length = list[extIndex].length;
				ext.offset += ext.skip;
				continue;
			}
			++extIndex;
			if(extIndex == extCount) {
				break;
			}
			ext = list[extIndex];
		}

		return res;
	}

private:
	Storage::Partition part;
	IFS::Extent ext{};
	IFS::Extent* list;
	uint16_t extCount;
	uint16_t extIndex{0};
};

} // namespace

class ExtentsTest : public TestGroup
{
public:
	ExtentsTest() : TestGroup(_F("File Extents"))
	{
	}

	void execute() override
	{
		TEST_CASE("FWFS tests")
		{
			fwfs_mount();
			extentsTest();
		}

		TEST_CASE("LittleFS tests")
		{
			lfs_mount();
			extentsTest();
		}

		TEST_CASE("SPIFFS tests")
		{
			spiffs_mount();
			extentsTest();
		}
	}

	void extentsTest()
	{
		Directory dir;
		REQUIRE(dir.open());
		while(dir.next()) {
			auto& stat = dir.stat();
			if(stat.isDir()) {
				continue;
			}
			Serial << "- " << stat.name << " " << stat.size << " bytes." << endl;
			File f;
			CHECK(f.open(stat.name));

			int extcount = f.getExtents(nullptr, nullptr, 0);
			if(extcount < 0) {
				Serial << F("Failed to get extents: ") << fileGetErrorString(extcount) << endl;
				if(extcount != IFS::Error::NotSupported) {
					TEST_ASSERT(false);
				}
			}
			if(extcount <= 0) {
				Serial << F("Skipping file") << endl;
				continue;
			}
			std::unique_ptr<IFS::Extent[]> list{new IFS::Extent[extcount]};
			CHECK(list);
			Storage::Partition part;
			int res = f.getExtents(&part, list.get(), extcount);
			CHECK_EQ(res, extcount);
			Serial << "  " << extcount << " extents" << endl;
			for(int i = 0; i < extcount; ++i) {
				auto& ext = list[i];
				debug_i("    #%u @ 0x%08x, len %u, skip %u, repeat %u", i, ext.offset, ext.length, ext.skip,
						ext.repeat);
			}

			f.seek(0, SeekOrigin::Start);
			ExtentStream es(part, list.get(), extcount);
			for(uint32_t offset = 0;;) {
				char buf1[512];
				auto len = f.read(buf1, sizeof(buf1));
				if(len == 0) {
					break;
				}
				char buf2[512];
				CHECK_EQ(es.read(buf2, len), len);

				if(memcmp(buf1, buf2, len) != 0) {
					debug_e("Fail @0x%08x", offset);
					m_printHex("BUF1", buf1, len, -1, 48);
					m_printHex("BUF2", buf2, len, -1, 48);
					TEST_ASSERT(false);
				}

				offset += len;
			}
		}
	}
};

void REGISTER_TEST(Extents)
{
	registerGroup<ExtentsTest>();
}

/*

lfs partition @ 0xB0000
"empty" @ 0xB1A04 - block 1, offset 2564
"empty" @ 0xB1C04 - block 1, offset 3076
3376 = 0xD30

file.id = 19
LFS_MKTAG(LFS_TYPE_INLINESTRUCT, file->id, 0)
	-> LFS_MKTAG(0x201, 19, 0)
	-> (0x201 << 20) | (19 << 10) | 0
	-> 0x20104C00
*/
