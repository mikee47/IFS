/**
 * xattr.cpp
 *
 * Created on: 16 Feb 2021
 *
 * Copyright 2019 mikee47 <mike@sillyhouse.net>
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
 ****/

#include "xattr.h"
#include <io.h>
#include <errno.h>
#include <ddk/ntifs.h>

namespace
{
constexpr size_t nameSize{32};
constexpr size_t contentSize{1024};

NTSTATUS getExtendedAttribute(HANDLE hFile, const char* name, void* buffer, size_t bufSize, size_t& length)
{
	char buf[sizeof(FILE_FULL_EA_INFORMATION) + nameSize + contentSize];
	auto get = reinterpret_cast<FILE_GET_EA_INFORMATION*>(buf);
	get->NextEntryOffset = 0;
	get->EaNameLength = strlen(name);
	memcpy(get->EaName, name, get->EaNameLength + 1);
	auto get_len = offsetof(FILE_GET_EA_INFORMATION, EaName) + get->EaNameLength + 1;
	IO_STATUS_BLOCK ioStatus{};
	NTSTATUS status = NtQueryEaFile(hFile, &ioStatus, buf, sizeof(buf), true, get, get_len, nullptr, true);
	if(status == STATUS_SUCCESS) {
		auto info = reinterpret_cast<FILE_FULL_EA_INFORMATION*>(buf);
		memcpy(buffer, &info->EaName[info->EaNameLength + 1], info->EaValueLength);
		length = info->EaValueLength;
	} else {
		length = 0;
	}
	return status;
}

NTSTATUS setExtendedAttribute(HANDLE hFile, const char* name, const void* value, size_t len)
{
	char buf[sizeof(FILE_FULL_EA_INFORMATION) + nameSize + contentSize];
	auto info = reinterpret_cast<FILE_FULL_EA_INFORMATION*>(buf);
	info->NextEntryOffset = 0;
	info->Flags = 0;
	info->EaNameLength = strlen(name);
	memcpy(info->EaName, name, info->EaNameLength + 1);
	info->EaValueLength = len;
	memcpy(&info->EaName[info->EaNameLength + 1], value, len);
	IO_STATUS_BLOCK ioStatus{};
	auto buflen = offsetof(FILE_FULL_EA_INFORMATION, EaName) + info->EaNameLength + 1 + info->EaValueLength;
	return NtSetEaFile(hFile, &ioStatus, buf, buflen);
}

size_t queryExtendedAttributeNames(HANDLE hFile, char* buffer, size_t bufSize)
{
	size_t length{0};

	for(unsigned i = 0;; ++i) {
		IO_STATUS_BLOCK ioStatus{};
		char buf[sizeof(FILE_FULL_EA_INFORMATION) + nameSize + contentSize];
		auto res = NtQueryEaFile(hFile, &ioStatus, buf, sizeof(buf), true, nullptr, 0, nullptr, i == 0);
		if(res != STATUS_SUCCESS) {
			// res == STATUS_NO_EAS_ON_FILE
			// res == STATUS_NO_MORE_EAS when finished
			break;
		}

		auto& info = *reinterpret_cast<FILE_FULL_EA_INFORMATION*>(buf);
		if(memcmp(info.EaName, "LX.", 3) != 0) {
			continue;
		}
		auto name = info.EaName + 3;
		auto namelen = info.EaNameLength - 3;
		if(length + namelen > bufSize) {
			break;
		}
		memcpy(buffer + length, name, namelen);
		length += namelen;
		if(length == bufSize) {
			break;
		}
		buffer[length++] = '\0';
	}
	return length;
}

int getErrno(NTSTATUS status)
{
	switch(status) {
	case STATUS_SUCCESS:
		return 0;

	case STATUS_NOT_SUPPORTED:
	case STATUS_NOT_IMPLEMENTED:
	case STATUS_EAS_NOT_SUPPORTED:
		return ENOSYS;

	case STATUS_INVALID_EA_NAME:
	case STATUS_INVALID_EA_FLAG:
	case STATUS_NONEXISTENT_EA_ENTRY:
	case STATUS_EA_CORRUPT_ERROR:
		return EINVAL;

	case STATUS_INSUFFICIENT_RESOURCES:
	case STATUS_EA_LIST_INCONSISTENT:
	case STATUS_EA_TOO_LARGE:
	default:
		return EIO;
	}
}

struct EaBuffer {
	char name[nameSize];
	char content[contentSize];

	EaBuffer(const char* name)
	{
		memcpy(this->name, "lx.", 3);
		strcpy(&this->name[3], name);
	}
};

HANDLE getHandle(int file)
{
	return HANDLE(_get_osfhandle(file));
}

const char* lxea_prefix{"lxea"};
constexpr size_t lxea_prefix_length{4};

} // namespace

int fgetxattr(int file, const char* name, void* value, size_t size)
{
	EaBuffer buffer(name);
	size_t length;
	auto status = getExtendedAttribute(getHandle(file), buffer.name, buffer.content, contentSize, length);
	errno = getErrno(status);
	if(status != STATUS_SUCCESS) {
		return -1;
	}
	if(length < lxea_prefix_length || memcmp(buffer.content, lxea_prefix, lxea_prefix_length) != 0) {
		// No data
		return 0;
	}

	length -= lxea_prefix_length;
	if(length > size) {
		length = size;
	}
	memcpy(value, &buffer.content[lxea_prefix_length], length);

	return length;
}

int fsetxattr(int file, const char* name, const void* value, size_t size, int flags)
{
	(void)flags;
	EaBuffer buffer(name);
	memcpy(buffer.content, lxea_prefix, lxea_prefix_length);
	memcpy(&buffer.content[lxea_prefix_length], value, size);
	auto status = setExtendedAttribute(getHandle(file), buffer.name, buffer.content, size + lxea_prefix_length);
	errno = getErrno(status);
	return (status == STATUS_SUCCESS) ? 0 : -1;
}

int flistxattr(int file, char* namebuf, size_t size)
{
	return queryExtendedAttributeNames(getHandle(file), namebuf, size);
}

int fgetattr(int file, uint32_t& attr)
{
	FILE_BASIC_INFORMATION info;
	IO_STATUS_BLOCK ioStatus;
	auto status = NtQueryInformationFile(getHandle(file), &ioStatus, &info, sizeof(info), FileBasicInformation);
	if(status != STATUS_SUCCESS) {
		errno = getErrno(status);
		return -1;
	}

	attr = info.FileAttributes;
	return 0;
}
