#include <FsTest.h>

String getErrorString(FileSystem* fs, int err)
{
	if(fs == nullptr) {
		return IFS::Error::toString(err);
	} else {
		return fs->getErrorString(err);
	}
}

String toString(Flag f)
{
	switch(f) {
#define XX(name, desc)                                                                                                 \
	case Flag::name:                                                                                                   \
		return F(#name);
		FLAG_MAP(XX)
#undef XX
	default:
		return nullptr;
	}
}

void printFsInfo(FileSystem* fs)
{
	char namebuf[256];
	FileSystem::Info info(namebuf, sizeof(namebuf));
	int res = fs->getinfo(info);
	if(res < 0) {
		debug_e("fileSystemGetInfo(): %s", fs->getErrorString(res).c_str());
		return;
	}

	debug_i("type:       %s", toString(info.type).c_str());
	debug_i("partition:  %s", info.partition.name().c_str());
	if(info.partition) {
		debug_i(" type:      %s", info.partition.longTypeString().c_str());
		debug_i(" device:    %s", info.partition.getDeviceName().c_str());
	}
	debug_i("maxNameLength: %u", info.maxNameLength);
	debug_i("maxPathLength: %u", info.maxPathLength);
	debug_i("attr:          %s", toString(info.attr).c_str());
	debug_i("volumeID:      0x%08X", info.volumeID);
	debug_i("name:          %s", info.name.buffer);
	debug_i("volumeSize:    %u", info.volumeSize);
	debug_i("freeSpace:     %u", info.freeSpace);
}

void printFileInfo(const IFS::Stat& stat)
{
	FileSystem::Info info;
	stat.fs->getinfo(info);
	debug_i("%-50s %8u %s #0x%08x %s %s [%s] {%s, %u}", stat.name.buffer, stat.size, toString(info.type).c_str(),
			stat.id, timeToStr(stat.mtime, " ").c_str(), toString(stat.acl).c_str(), toString(stat.attr).c_str(),
			toString(stat.compression.type).c_str(), stat.compression.originalSize);
}

// Displays as local time
String timeToStr(time_t t, const char* dtsep)
{
	struct tm* tm = localtime(&t);
	if(tm == nullptr) {
		return nullptr;
	}
	char buffer[64];
	m_snprintf(buffer, sizeof(buffer), "%02u/%02u/%04u%s%02u:%02u:%02u", tm->tm_mday, tm->tm_mon + 1,
			   1900 + tm->tm_year, dtsep, tm->tm_hour, tm->tm_min, tm->tm_sec);
	return buffer;
}

int listdir(IFS::FileSystem* fs, const String& path, Flags flags)
{
	debug_i("$ ls %s", path.c_str());

	IFS::Directory dir(fs);
	if(dir.open(path)) {
		while(dir.next()) {
			printFileInfo(dir.stat());
		}

		if(flags[Flag::recurse]) {
			dir.rewind();
			while(dir.next()) {
				if(dir.stat().attr[IFS::FileAttribute::Directory]) {
					String subdir = path;
					if(subdir.length() != 0) {
						subdir += '/';
					}
					subdir += dir.stat().name;
					listdir(fs, subdir, flags);
				}
			}
		}
	}

	return dir.getLastError();
}

void listPartitions()
{
	Serial.println();
	Serial.println(_F("Registered partitions:"));
	for(auto it = Storage::findPartition(); it; ++it) {
		Serial.print("- ");
		Serial.print((*it).name());
		Serial.print(" on ");
		Serial.println((*it).getDeviceName());
	}
	Serial.println();
}

void listDevices()
{
	Serial.println();
	Serial.println(_F("Registered storage devices:"));
	for(auto& dev : Storage::getDevices()) {
		Serial.print("  name = '");
		Serial.print(dev.getName());
		Serial.print(_F("', type = "));
		Serial.print(toString(dev.getType()));
		Serial.print(_F(", size = 0x"));
		Serial.println(dev.getSize(), HEX);
		for(auto part : dev.partitions()) {
			Serial.print("    Part '");
			Serial.print(part.name());
			Serial.print(_F("', type = "));
			Serial.print(part.longTypeString());
			Serial.print(_F(", address = 0x"));
			Serial.print(part.address(), HEX);
			Serial.print(_F(", size = 0x"));
			Serial.println(part.size(), HEX);
		}
	}
	Serial.println();
}
