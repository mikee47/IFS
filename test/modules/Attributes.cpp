/*
 * Attributes.cpp
 *
 *  Created on: 19 Feb 2021
 *      Author: mikee47
 */

#include <FsTest.h>
#include <Spiffs.h>
#include <LittleFS.h>

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
		TEST_CASE("LittleFS tests")
		{
			lfs_mount();
			attributeTest();
			userAttributeTest();
		}

		TEST_CASE("SPIFFS tests")
		{
			spiffs_mount();
			attributeTest();
			userAttributeTest();
		}

#ifdef ARCH_HOST
		hostAttributeTest();
#endif
	}

	void attributeTest()
	{
		DEFINE_FSTR_LOCAL(filename, "test.txt");
		DEFINE_FSTR_LOCAL(content, "This is some test content");

		auto fs = getFileSystem();
		CHECK(fs != nullptr);
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

	void userAttributeTest()
	{
		DEFINE_FSTR_LOCAL(filename, "usertest.txt")
		DEFINE_FSTR_LOCAL(coolDonkeys, "cool donkeys")
		DEFINE_FSTR_LOCAL(comment, "Donkeys are cool")

		auto fs = getFileSystem();
		CHECK(fs != nullptr);
		fs->setContent(filename, "empty");
		int err = fs->setUserAttribute(filename, 0, coolDonkeys);
		REQUIRE_EQ(err, FS_OK);
		String s = fs->getUserAttribute(filename, 0);
		REQUIRE_EQ(s, coolDonkeys);

		TEST_CASE("Set comment")
		{
			int err = fs->setAttribute(filename, IFS::AttributeTag::Comment, comment);
			if(err == FS_OK) {
				String s = fs->getAttribute(filename, IFS::AttributeTag::Comment);
				REQUIRE_EQ(s, comment);
			} else {
				REQUIRE_EQ(err, IFS::Error::NotSupported);
			}
		}

		TEST_CASE("Set user attributes")
		{
			File f;
			REQUIRE(f.open(filename, File::Create | File::ReadWrite));

			if(!f.setUserAttribute(0, "hello")) {
				debug_e("setUserAttribute(): %s", f.getLastErrorString().c_str());
				TEST_ASSERT(false);
			}
			REQUIRE(f.getUserAttribute(0) == "hello");

			if(!f.setUserAttribute(0xAA, "goodbye")) {
				debug_e("setUserAttribute(): %s", f.getLastErrorString().c_str());
				TEST_ASSERT(false);
			}
			REQUIRE(f.getUserAttribute(0xAA) == "goodbye");

			if(!f.setUserAttribute(0, "car")) {
				debug_e("setUserAttribute(): %s", f.getLastErrorString().c_str());
				TEST_ASSERT(false);
			}
			REQUIRE(f.getUserAttribute(0) == "car");

			if(!f.setUserAttribute(0xAA, "")) {
				debug_e("setUserAttribute(): %s", f.getLastErrorString().c_str());
				TEST_ASSERT(false);
			}
			REQUIRE(f.getUserAttribute(0xAA) == "");

			if(!f.removeUserAttribute(0)) {
				debug_e("removeUserAttribute(): %s", f.getLastErrorString().c_str());
				TEST_ASSERT(false);
			}
			REQUIRE(!f.getUserAttribute(0));
		}
	}

#ifdef ARCH_HOST
	void hostAttributeTest()
	{
		auto& hostfs = IFS::Host::getFileSystem();

		TEST_CASE("Host attributes")
		{
			IFS::File f(&hostfs);
			f.open("attrtest1.txt", File::Create | File::ReadWrite);
			CHECK(f);
			f.write("Hello", 5);
			f.setacl({IFS::UserRole::Guest, IFS::UserRole::Manager});
			f.setUserAttribute(0, "I am a user");
			f.setUserAttribute(10, "I am not a user");
		}

		TEST_CASE("Directory listing")
		{
			listdir(&hostfs, nullptr, 0); //Flag::recurse);
		}

		TEST_CASE("Attribute enumeration")
		{
			IFS::File f(&hostfs);
			f.open("attrtest1.txt");
			auto callback = [](IFS::AttributeEnum& e) -> bool {
				m_printHex(toString(e.tag).c_str(), e.buffer, e.size);
				return true;
			};
			char buffer[256];
			f.enumAttributes(callback, buffer, sizeof(buffer));
		}
	}
#endif
};

void REGISTER_TEST(Attributes)
{
	registerGroup<AttributesTest>();
}
