#pragma once

#include <SmingTest.h>
#include <IFS/FileSystem.h>
#include <IFS/Helpers.h>
#include <IFS/Directory.h>

#define debug_ifs(fs, err, func, ...)                                                                                  \
	do {                                                                                                               \
		int errorCode = err;                                                                                           \
		(void)errorCode;                                                                                               \
		debug_e(func ": %s (%d)", ##__VA_ARGS__, getErrorString(fs, errorCode).c_str(), err);                          \
	} while(0)

#define debug_file(file, func, ...)                                                                                    \
	do {                                                                                                               \
		debug_e(func ": %s", ##__VA_ARGS__, file.getLastErrorString().c_str());                              \
	} while(0)

using IFileSystem = IFS::IFileSystem;
using FileSystem = IFS::FileSystem;

#define FLAG_MAP(XX)                                                                                                   \
	XX(hybrid, "Use hybrid filesystem, enabling write tests")                                                          \
	XX(readFileTest, "Read content of every file")                                                                     \
	XX(writeThroughTest, "With hybrid filesystem, 'touch' files to propagate them to SPIFFS from FWFS")                \
	XX(recurse, "Recurse sub-directories")

enum class Flag {
#define XX(name, desc) name,
	FLAG_MAP(XX)
#undef XX
};

using Flags = BitSet<uint32_t, Flag, 3>;

String getErrorString(IFileSystem* fs, int err);

String toString(Flag f);

void printFsInfo(IFileSystem* fs);

void printFileInfo(const IFS::Stat& stat);

// Displays as local time
String timeToStr(time_t t, const char* dtsep);

int listdir(IFileSystem* fs, const String& path, Flags flags = 0);

void listPartitions();
void listDevices();