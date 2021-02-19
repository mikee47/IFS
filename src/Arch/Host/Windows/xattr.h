/*
 * xattr.h
 *
 *  Created on: 16 Feb 2021
 *      Author: mikee47
 *
 * Extended Attributes
 */

#include <cstddef>
#include <cstdint>

int fgetxattr(int file, const char* name, void* value, size_t size);
int fsetxattr(int file, const char* name, const void* value, size_t size, int flags);

// Get Win32 attribute bits (FILE_ATTRIBUTE_xxx)
int fgetattr(int file, uint32_t& attr);
