/*
 * Attributes.cpp
 *
 *  Created on: 19 Feb 2021
 *      Author: mikee47
 */

#include <FsTest.h>

#ifdef ARCH_HOST
#include <IFS/Host/FileSystem.h>
#endif

class AttributesTest : public TestGroup
{
public:
	AttributesTest() : TestGroup(_F("File Attributes"))
	{
	}

	void execute() override
	{
		attributeTest();
#ifdef ARCH_HOST
		hostAttributeTest();
#endif
	}

	void attributeTest()
	{
		DEFINE_FSTR_LOCAL(filename, "test.txt");
		DEFINE_FSTR_LOCAL(content, "This is some test content");

		spiffs_mount();

		auto fs = getFileSystem();
		listdir(fs, nullptr);

		// If our test file already exists, make sure it's not read-only
		int res = fileSetAttr(filename, 0);
		debug_ifs(fs, res, "fileSetAttr()");

		FileHandle f;

		TEST_CASE("Create test file")
		{
			f = fileOpen(filename, File::CreateNewAlways | File::ReadWrite);
			debug_ifs(fs, f, "fileOpen(CreateNewAlways)");
			REQUIRE(f >= 0);
			res = fileWrite(f, String(content).c_str(), content.length());
			debug_ifs(fs, res, "fileWrite()");
			REQUIRE(res >= 0);
			fileClose(f);
		}

		// Now set the file to read-only and try again
		TEST_CASE("Set file to read-only")
		{
			res = fileSetAttr(filename, IFS::FileAttribute::ReadOnly);
			debug_ifs(fs, res, "fileSetAttr()");
			REQUIRE(res == FS_OK);
		}

		TEST_CASE("Open read-only file with write permission")
		{
			f = fileOpen(filename, File::ReadWrite);
			debug_ifs(fs, f, "fileOpen(ReadWrite)");
			REQUIRE(f < 0);
		}

		TEST_CASE("Verify file can be read normally, with writes still failing")
		{
			f = fileOpen(filename, File::ReadOnly);
			debug_ifs(fs, f, "fileOpen(ReadOnly)");
			REQUIRE(f >= 0);

			res = fileWrite(f, "test", 4);
			debug_ifs(fs, res, "fileWrite()");
			REQUIRE(res < 0);

			char buffer[256];
			res = fileRead(f, buffer, sizeof(buffer));
			REQUIRE_EQ(size_t(res), content.length());
			REQUIRE(content.equals(buffer, content.length()));
		}

		TEST_CASE("Attempt to delete read-only file")
		{
			res = fileDelete(f);
			debug_ifs(fs, res, "fileDelete(handle)");
			REQUIRE(res < 0);

			fileClose(f);

			res = fileDelete(filename);
			debug_ifs(fs, res, "fileDelete(filename)");
			REQUIRE(res < 0);
		}

		TEST_CASE("Remove read-only attribute and delete")
		{
			res = fileSetAttr(filename, 0);
			debug_ifs(fs, res, "fileSetAttr()");
			REQUIRE(res == FS_OK);

			res = fileDelete(filename);
			debug_ifs(fs, res, "fileDelete(filename)");
			REQUIRE(res == FS_OK);
		}
	}

#ifdef ARCH_HOST
	void hostAttributeTest()
	{
		auto& hostfs = IFS::Host::getFileSystem();
		auto f = hostfs.open("attrtest1.txt", File::Create | File::ReadWrite);
		CHECK(f >= 0);
		hostfs.write(f, "Hello", 5);
		hostfs.setacl(f, {IFS::UserRole::Guest, IFS::UserRole::Manager});
		hostfs.close(f);

		listdir(&hostfs, nullptr, Flag::recurse);
	}
#endif
};

void REGISTER_TEST(Attributes)
{
	registerGroup<AttributesTest>();
}
