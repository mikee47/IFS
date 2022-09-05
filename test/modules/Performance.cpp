/*
 * Performance.cpp
 *
 *  Created on: 24 April 2021
 *      Author: mikee47
 */

#include <FsTest.h>
#include <IFS/Helpers.h>
#include <LittleFS.h>
#include <Platform/Timers.h>

DEFINE_FSTR_LOCAL(TEST_READ_FILENAME, "apple-touch-icon-180x180.png")
DEFINE_FSTR_LOCAL(TEST_WRITE_FILENAME, "testwrite.png")
IMPORT_FSTR_LOCAL(TEST_CONTENT, PROJECT_DIR "/files/apple-touch-icon-180x180.png")

class PerformanceTest : public TestGroup
{
public:
	PerformanceTest() : TestGroup(_F("Performance tests"))
	{
	}

	void execute() override
	{
		TEST_CASE("SPIFFS benchmark")
		{
			fileSetFileSystem(nullptr);
			size_t heapSize = system_get_free_heap_size();
			spiffs_mount();
			printHeap(heapSize);
			benchmark(false);
		}

		TEST_CASE("LittleFS benchmark")
		{
			fileSetFileSystem(nullptr);
			size_t heapSize = system_get_free_heap_size();
			lfs_mount();
			printHeap(heapSize);
			benchmark(false);
		}

		TEST_CASE("FWFS benchmark")
		{
			fileSetFileSystem(nullptr);
			size_t heapSize = system_get_free_heap_size();
			fwfs_mount();
			printHeap(heapSize);
			benchmark(true);
		}
	}

	void printHeap(size_t initialHeapSize)
	{
		size_t used = initialHeapSize - system_get_free_heap_size();
		debug_i("Heap used: %u", used);
	}

	void profile(const String& func, unsigned iterations, Delegate<void()> callback)
	{
		OneShotFastUs timer;

		for(unsigned i = 0; i < iterations; ++i) {
			callback();
		}

		auto time = timer.elapsedTime();

		m_printf("%10s", func.c_str());
		Serial.print(_F(": Time taken for "));
		Serial.print(iterations);
		Serial.print(_F(" iterations = "));
		Serial.print(time.toString());
		Serial.print(_F(", timer per iteration = "));
		time.time = (time.time + iterations / 2) / iterations;
		Serial.println(time.toString());
	}

	void benchmark(bool readOnly)
	{
		String filename;
		if(readOnly) {
			filename = TEST_READ_FILENAME;
		} else {
			filename = TEST_WRITE_FILENAME;
			int size = fileSetContent(filename, TEST_CONTENT);
			CHECK(size > 0);
		}

		profile(F("open"), 100, [&]() {
			FileHandle file = fileOpen(filename);
			CHECK(file >= 0);
			fileClose(file);
		});

		FileHandle file = fileOpen(filename);
		CHECK(file >= 0);
		profile(F("seek"), 500, [&]() {
			int pos = fileSeek(file, 0, SeekOrigin::Start);
			CHECK(pos == 0);
			int size = fileSeek(file, 0, SeekOrigin::End);
			CHECK(size > 0);
			pos = fileSeek(file, -size / 2, SeekOrigin::Current);
			CHECK(pos == size - (size / 2));
		});

		profile(F("seek/read"), 500, [&]() {
			int res = fileSeek(file, 0, SeekOrigin::Start);
			CHECK(res == 0);
			uint8_t buffer[1024];
			res = fileRead(file, buffer, sizeof(buffer));
			CHECK(res == sizeof(buffer));
		});
		fileClose(file);

		if(readOnly) {
			return;
		}

		file = fileOpen(filename, File::ReadWrite);
		debug_e("fileOpen(ReadWrite): %s", fileGetErrorString(file).c_str());
		CHECK(file >= 0);
		FileStat stat;
		int err = fileStats(file, stat);
		CHECK(err >= 0);
		Serial.println(stat);
		CHECK(file >= 0);
		uint8_t buffer[1024];
		fileRead(file, buffer, sizeof(buffer));
		profile(F("seek/write"), 20, [&]() {
			int res = fileSeek(file, 0, SeekOrigin::Start);
			CHECK(res == 0);
			res = fileWrite(file, buffer, sizeof(buffer));
			if(res != sizeof(buffer)) {
				debug_e("fileWrite(): %s", fileGetErrorString(res).c_str());
				TEST_ASSERT(false);
			}
		});
		fileClose(file);
	}
};

void REGISTER_TEST(Performance)
{
	registerGroup<PerformanceTest>();
}
