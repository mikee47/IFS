/*
 * Links.cpp
 *
 *  Created on: 24 April 2021
 *      Author: mikee47
 */

#include <FsTest.h>

DEFINE_FSTR_LOCAL(FS_MOUNTPOINT_PATH, "spiffs")
DEFINE_FSTR_LOCAL(FS_MOUNTPOINT_NAME, "spiffs")

class LinksTest : public TestGroup
{
public:
	LinksTest() : TestGroup(_F("Links"))
	{
	}

	void execute() override
	{
		int err;

		REQUIRE(fwfs_mount());
		auto fs = getFileSystem();

		TEST_CASE("Mount SPIFFS")
		{
			auto part = Storage::findDefaultPartition(Storage::Partition::SubType::Data::spiffs);
			auto spiffs = IFS::createSpiffsFilesystem(part);
			REQUIRE(spiffs != nullptr);
			REQUIRE(spiffs->mount() == FS_OK);
			// fileSetMountPoint(0, spiffs);
			fs->setVolume(0, spiffs);
		}

		TEST_CASE("stat of mountpoint")
		{
			FileNameStat stat;
			err = fileStats(FS_MOUNTPOINT_PATH, stat);
			debug_ifs(fs, err, "stat");
			CHECK(err == FS_OK);
			printFileInfo(Serial, stat);
			REQUIRE(FS_MOUNTPOINT_NAME == stat.name);
			REQUIRE(stat.attr == FileAttribute::Directory + FileAttribute::MountPoint);
		}

		int32_t testValue{12345};
		String path = FS_MOUNTPOINT_PATH;
		path += _F("/test");

		TEST_CASE("Fail write operations on mountpoint root")
		{
			err = fs->rename(FS_MOUNTPOINT_PATH, path);
			debug_ifs(fs, err, "rename");
			REQUIRE(err == IFS::Error::ReadOnly);

			err = fs->remove(FS_MOUNTPOINT_PATH);
			debug_ifs(fs, err, "remove");
			REQUIRE(err == IFS::Error::ReadOnly);

			err = fs->mkdir(FS_MOUNTPOINT_PATH);
			debug_ifs(fs, err, "mkdir");
			REQUIRE(err == IFS::Error::ReadOnly);

			err = fs->setUserAttribute(FS_MOUNTPOINT_PATH, 0, &testValue, sizeof(testValue));
			debug_ifs(fs, err, "setUserAttribute");
			REQUIRE(err == IFS::Error::ReadOnly);
		}

		TEST_CASE("Create test file in mountpoint")
		{
			err = fs->setContent(path, nullptr);
			debug_ifs(fs, err, "setContent");
			REQUIRE(err == FS_OK);
		}

		TEST_CASE("Check setxattr for file in mountpoint")
		{
			err = fs->setUserAttribute(path, 0, &testValue, sizeof(testValue));
			debug_ifs(fs, err, "setUserAttribute");
			REQUIRE(err == FS_OK);
		}

		TEST_CASE("Check getxattr for file in mountpoint")
		{
			int32_t x{0};
			err = fs->getUserAttribute(path, 0, &x, sizeof(x));
			debug_ifs(fs, err, "getUserAttribute");
			REQUIRE(err == sizeof(x));
			REQUIRE_EQ(x, testValue);
		}
	}
};

void REGISTER_TEST(Links)
{
	registerGroup<LinksTest>();
}
