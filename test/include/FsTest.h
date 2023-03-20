#pragma once

#include <SmingTest.h>
#include <IFS/FileSystem.h>
#include <IFS/Helpers.h>
#include <IFS/Directory.h>
#include <IFS/Debug.h>
#include <Storage/Debug.h>

using namespace IFS::Debug;
using namespace Storage::Debug;

#define debug_ifs(fs, err, func, ...)                                                                                  \
	do {                                                                                                               \
		int errorCode = err;                                                                                           \
		(void)errorCode;                                                                                               \
		debug_e(func ": %s (%d)", ##__VA_ARGS__, getErrorString(fs, errorCode).c_str(), err);                          \
	} while(0)

#define debug_file(file, func, ...)                                                                                    \
	do {                                                                                                               \
		debug_e(func ": %s", ##__VA_ARGS__, file.getLastErrorString().c_str());                                        \
	} while(0)

using FileSystem = IFS::FileSystem;

#define FLAG_MAP(XX)                                                                                                   \
	XX(readFileTest, "Read content of every file")                                                                     \
	XX(writeThroughTest, "With hybrid filesystem, 'touch' files to propagate them to SPIFFS from FWFS")

enum class Flag {
#define XX(name, desc) name,
	FLAG_MAP(XX)
#undef XX
};

using Flags = BitSet<uint32_t, Flag, 3>;

String getErrorString(FileSystem* fs, int err);

String toString(Flag f);
