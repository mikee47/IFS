/**
 * ArchiveStream.cpp
 *
 * Copyright 2021 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the IFS Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <IFS/FWFS/ArchiveStream.h>
#include <Data/Stream/IFS/FileStream.h>

namespace IFS
{
namespace FWFS
{
constexpr size_t maxInlineSize{255};

int ArchiveStream::FileInfo::setAttribute(AttributeTag tag, const void* data, size_t size)
{
	return dir.addAttribute(tag, data, size);
}

uint16_t ArchiveStream::readMemoryBlock(char* data, int bufSize)
{
	if(source == nullptr || source->isFinished()) {
		if(!fillBuffers()) {
			return 0;
		}
	}

	assert(source != nullptr);
	return source->readMemoryBlock(data, bufSize);
}

bool ArchiveStream::fillBuffers()
{
	auto gotoEnd = [&]() {
		buffer.write(FWFILESYS_END_MARKER);
		queueStream(&buffer, State::end);
	};

	switch(state) {
	case State::idle:
		if(volumeInfo.creationTime == 0) {
			volumeInfo.creationTime = fsGetTimeUTC();
		}
		buffer.write(FWFILESYS_START_MARKER);
		queueStream(&buffer, State::start);
		break;

	case State::start:
		if(!openRootDirectory()) {
			gotoEnd();
		}
		break;

	case State::dataHeader:
		sendDataContent();
		break;

	case State::dataContent:
		sendDataHeader();
		break;

	case State::fileHeader:
		if(!readDirectory()) {
			closeDirectory();
		}
		break;

	case State::dirHeader:
		if(level == 0) {
			getVolume();
			break;
		}
		if(!readDirectory()) {
			closeDirectory();
		}
		break;

	case State::volumeHeader:
		gotoEnd();
		break;

	case State::end:
		state = State::done;
		return false;

	case State::done:
	case State::error:
		return false;
	}

	return true;
}

int ArchiveStream::seekFrom(int offset, SeekOrigin origin)
{
	if(origin == SeekOrigin::Current) {
		return source ? source->seekFrom(offset, origin) : -1;
	}
	if(origin == SeekOrigin::Start && offset == 0) {
		reset();
		return 0;
	}
	return -1;
}

void ArchiveStream::reset()
{
	if(state == State::idle) {
		return;
	}

	GET_FS()

	source = nullptr;
	encoder.reset();
	buffer.clear();
	for(unsigned i = 0; i <= level; ++i) {
		auto& dir = directories[i];
		fs->closedir(dir.handle);
		dir.handle = nullptr;
		dir.reset();
	}
	level = 0;
	streamOffset = queuedSize = 0;
	state = State::idle;
}

void ArchiveStream::queueStream(IDataSourceStream* stream, State newState)
{
	streamOffset += queuedSize;
	assert(stream != nullptr);
	int len = stream->available();
	assert(len > 0);
	source = stream;
	state = newState;
	queuedSize = len;
}

void ArchiveStream::openDirectory(const Stat& stat)
{
	GET_FS()

	auto& dir = directories[level];
	if(level > 0) {
		dir.namelen = 0;
		if(currentPath.length() > 0) {
			currentPath += '/';
			++dir.namelen;
		}
		currentPath.concat(stat.name.c_str(), stat.name.length);
		dir.namelen += stat.name.length;
	}
	debug_d("[FWFS] openDirectory('%s')", currentPath.c_str());

	// Directory header
	dir.type = stat.attr[FileAttribute::MountPoint] ? Object::Type::MountPoint : Object::Type::Directory;
	dir.createContent();
	dir.content->writeNamed(dir.type, stat.name.c_str(), stat.name.length, stat.mtime);

	auto file = fs->open(currentPath, OpenFlag::Read | OpenFlag::NoFollow);
	if(file < 0) {
		debug_w("[FWFS] Failed to open handle to directory '%s': %s", currentPath.c_str(),
				fs->getErrorString(file).c_str());
	} else {
		getAttributes(file, dir);
		fs->close(file);
	}

	++level;

	// By default, don't enumerate mountpoints
	if(stat.attr[FileAttribute::MountPoint] && !flags[Flag::IncludeMountPoints]) {
		closeDirectory();
		return;
	}

	int err = fs->opendir(currentPath, dir.handle);
	if(err < 0) {
		debug_w("[FWFS] opendir('%s'): %s", currentPath.c_str(), fs->getErrorString(err).c_str());
		closeDirectory();
		return;
	}

	if(!readDirectory()) {
		closeDirectory();
	}
}

bool ArchiveStream::openRootDirectory()
{
	GET_FS(false)

	// Root directory may be a mountpoint, so query stats from open handle rather than path
	assert(level == 0);
	debug_d("[FWFS] Root directory: '%s'", currentPath.c_str());
	int file = fs->open(currentPath);
	if(file < 0) {
		debug_e("[FWFS] open('%s'): %s", currentPath.c_str(), fs->getErrorString(file).c_str());
		return false;
	}
	Stat stat;
	int res = fs->fstat(file, stat);
	fs->close(file);
	if(res < 0) {
		debug_e("[FWFS] stat('%s'): %s", currentPath.c_str(), fs->getErrorString(res).c_str());
		return false;
	}

	if(!stat.isDir()) {
		debug_e("[FWFS] Not a directory: '%s'", currentPath.c_str());
		return false;
	}

	stat.name = currentPath;
	openDirectory(stat);
	return true;
}

bool ArchiveStream::readDirectory()
{
	GET_FS(false)

	buffer.clear();
	assert(level > 0);
	auto& dir = directories[level - 1];
	for(;;) {
		NameStat stat;
		int err = fs->readdir(dir.handle, stat);
		if(err < 0) {
			if(err != Error::NoMoreFiles) {
				debug_w("[FWFS] readdir: %s", fs->getErrorString(err).c_str());
			}
			return false;
		}

		if(!filterStat(stat)) {
			debug_d("[FWFS] Skipping %s", stat.name.c_str());
			continue;
		}

		debug_d("[FWFS] Entry: %s", stat.name.c_str());

		if(stat.isDir()) {
			openDirectory(stat);
			break;
		}

		if(readFileEntry(stat)) {
			break;
		}
	}

	return true;
}

bool ArchiveStream::readFileEntry(const Stat& stat)
{
	GET_FS(false)

	auto& entry = directories[level];
	entry.content.reset();
	String path = currentPath;
	if(path.length() > 0) {
		path += '/';
	}
	path.concat(stat.name.c_str(), stat.name.length);

	auto file = fs->open(path, OpenFlag::Read);
	if(file < 0) {
		debug_e("[FWFS] Error opening '%s': %s", path.c_str(), fs->getErrorString(file).c_str());
		return false;
	}

	// fileHeader
	entry.type = Object::Type::File;
	entry.createContent();
	entry.content->writeNamed(entry.type, stat.name.c_str(), stat.name.length, stat.mtime);

	FileInfo info{*this, entry, file, stat};
	encoder.reset(createEncoder(info));

	bool inlineData = !encoder && stat.size < maxInlineSize;

	getAttributes(file, entry);
	if(inlineData) {
		// Put data inline for small files
		entry.content->writeDataHeader(stat.size);
		uint8_t buffer[stat.size];
		fs->read(file, buffer, stat.size);
		fs->close(file);
		entry.content->write(buffer, stat.size);
		sendFileHeader();
	} else {
		if(!encoder) {
			// Default behaviour
			auto stream = new FileStream(fs);
			stream->attach(file, stat.size);
			encoder.reset(new BasicEncoder(stream));
		}
		sendDataHeader();
	}

	return true;
}

void ArchiveStream::sendDataHeader()
{
	dataBlock = encoder->getNextStream();
	if(dataBlock == nullptr) {
		sendFileHeader();
		return;
	}

	auto size = dataBlock->available();
	assert(size > 0);
	if(size <= 0) {
		sendFileHeader();
		return;
	}

	buffer.clear();
	auto type = buffer.writeDataHeader(size);
	queueStream(&buffer, State::dataHeader);

	// Add reference to file header
	auto& entry = directories[level];
	entry.content->writeRef(type, streamOffset);
}

void ArchiveStream::sendDataContent()
{
	assert(dataBlock != nullptr);
	if(dataBlock == nullptr) {
		sendFileHeader();
	} else {
		queueStream(dataBlock, State::dataContent);
		dataBlock = nullptr;
	}
}

void ArchiveStream::sendFileHeader()
{
	encoder.reset();
	auto& entry = directories[level];
	entry.content->fixupSize();
	queueStream(entry.content.get(), State::fileHeader);

	// Add reference to parent directory
	auto& parent = directories[level - 1];
	parent.content->writeRef(entry.type, streamOffset);
}

int ArchiveStream::DirInfo::addAttribute(AttributeTag tag, const void* data, size_t size)
{
	if(tag >= AttributeTag::User) {
		auto tagValue = unsigned(tag) - unsigned(AttributeTag::User);
		if(tagValue > 255) {
			return Error::BadParam;
		}
		constexpr size_t maxAttrSize{255 - 3};
		if(size > maxAttrSize) {
			debug_w("[FWFS] Truncating user attribute from %u to %u bytes", size, maxAttrSize);
			size = maxAttrSize;
		}
		Object hdr{};
		hdr.setType(Object::Type::UserAttribute);
		hdr.data8.userAttribute.tagValue = tagValue;
		hdr.data8.setContentSize(1 + size);
		content->write(hdr, 1, size);
		content->write(data, size);
		return FS_OK;
	}

	auto append = [&](Object::Type type) {
		Object hdr{};
		hdr.setType(type);
		hdr.data8.setContentSize(size);
		content->write(hdr, 0, size);
		content->write(data, size);
		return FS_OK;
	};

	switch(tag) {
	case AttributeTag::FileAttributes: {
		auto attr = static_cast<const uint8_t*>(data);
		auto objattr = getObjectAttributes(FileAttributes(*attr));
		data = &objattr;
		return append(Object::Type::ObjAttr);
	}
	case AttributeTag::ReadAce:
		return append(Object::Type::ReadACE);
	case AttributeTag::WriteAce:
		return append(Object::Type::WriteACE);
	case AttributeTag::Compression:
		return append(Object::Type::Compression);
	case AttributeTag::Md5Hash:
		return append(Object::Type::Md5Hash);
	case AttributeTag::VolumeIndex:
		return append(Object::Type::VolumeIndex);
	case AttributeTag::Comment:
		return append(Object::Type::Comment);
	default:
		return Error::BadParam;
	}
}

int ArchiveStream::getAttributes(FileHandle file, DirInfo& entry)
{
	auto callback = [&](AttributeEnum& e) -> bool {
		if(e.tag != AttributeTag::VolumeIndex || !flags[Flag::IncludeMountPoints]) {
			entry.addAttribute(e.tag, e.buffer, e.size);
		}
		return true;
	};
	char attributeBuffer[1024];
	GET_FS(Error::NoFileSystem)
	return fs->fenumxattr(file, callback, attributeBuffer, sizeof(attributeBuffer));
}

void ArchiveStream::closeDirectory()
{
	GET_FS()

	assert(level > 0);
	directories[level].reset();
	--level;
	auto& dir = directories[level];
	fs->closedir(dir.handle);
	dir.handle = nullptr;
	assert(currentPath.length() >= dir.namelen);
	currentPath.setLength(currentPath.length() - dir.namelen);

	dir.content->fixupSize();
	queueStream(dir.content.get(), State::dirHeader);

	// Add entry for this directory to parent
	if(level > 0) {
		auto& parent = directories[level - 1];
		parent.content->writeRef(dir.type, streamOffset);
	}
}

void ArchiveStream::getVolume()
{
	buffer.writeNamed(Object::Type::Volume, volumeInfo.name.c_str(), volumeInfo.name.length(), volumeInfo.creationTime);

	// Volume ID
	Object hdr;
	hdr.setType(Object::Type::ID32);
	hdr.data8.id32.value = volumeInfo.id;
	hdr.data8.setContentSize(sizeof(uint32_t));
	buffer.write(hdr, sizeof(uint32_t), 0);

	// Last object written was root directory
	buffer.writeRef(Object::Type::Directory, streamOffset);
	buffer.fixupSize();

	// End
	hdr.setType(Object::Type::End);
	hdr.data8.end.checksum = 0; // not currently used
	hdr.data8.setContentSize(sizeof(uint32_t));
	buffer.write(hdr, sizeof(uint32_t), 0);

	queueStream(&buffer, State::volumeHeader);
}

} // namespace FWFS
} // namespace IFS
