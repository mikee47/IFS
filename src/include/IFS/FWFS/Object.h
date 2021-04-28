/**
 * Object.h
 * Basic definitions for FW file system structure.
 *
 *  Created on: 7 Aug 2018
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
 *
 * A filesystem image is basically:
 *
 * 	uint32_t START_MARKER;
 * 	Object Objects[];
 * 	Object EndObject;
 * 	uint32_t END_MARKER;
 *
 * An FWFS Object is a variable-length structure which can be read as either 1 or 2 words,
 * depending on the type.
 * Objects always start on a word boundary.
 * File and directory objects are both emitted as 'named' objects which contain a list of
 * zero or more child objects. When the image is built the child objects are emitted first,
 * followed by the parent. This puts the root directory at the end of the image.
 * This arrangement means an image can be generated without having to back-track and rewrite
 * headers.
 *
 * Child objects can be of any type. A directory object will mostly contain other file and
 * directory objects. File data is stored in a data object, not in the file object itself.
 * This is usually found following the file object, but it doesn't have to be.
 * Any object may be referenced by zero or more named object(s). For example, file links/aliases
 * can contain references to the same file data.
 * A file object may contain multiple data objects. These are treated as a contiguous block for file operations.
 * This would potentially allow a file system builder to place common file blocks into shared data objects.
 *
 * Object names are from 0 to 255 characters, inclusive. The root directory has a zero-length name.
 * Paths lengths are unlimited.
 * '/' is used as the path separator. It informs the filesystem of the parent/child relationship
 * between one directory object and a sub-ordinate.
 * ':' is used as the file stream separator. It performs the equivalent of the path separator for
 * non-directory named objects. For exaample, file object may contain named objects accessible thus:
 * 	index.html:info opens a handle to a named object called 'info' belonging to index.html.
 *
 * OK, I admit it; this is pinched from NTFS; but it's such a cool idea. Applications can use it
 * to attach custom data to their files without limitation.
 *
 * As a side note, of course FWFS is read-only. More precisely, it only supports random reading
 * of files, not random writing. Serial writing is supported in the form of image creation.
 *
 * For SPIFFS, IFS is a wrapper. The metadata features are supported using SPIFFS metadata.
 * An alternative approach is to implement every named object as a SPIFFS file. We'd then get
 * all the features of FWFS in a rewritable system, with all the benefits of SPIFFS wear-levelling.
 *
 * Objects are identified by their index, but it's not stored in the image. Instead, it's
 * tracked via internal object descriptor.
 *
 * To optimise lookups, an object table can be stored at the end of the image. This is
 * just an array of 32-bit image offsets so that an object can be located instantly on
 * large volumes. This will be optional as it can consume significant space.
 *
 */

#pragma once

#include "../TimeStamp.h"
#include "../FileAttributes.h"
#include "../Compression.h"
#include "../UserRole.h"

namespace IFS
{
namespace FWFS
{
/*
 * Helper template function to get the next object pointer at (current + offset)
 * casting as required for the given return type.
 */
template <typename T> static T at_offset(const void* current, int offset)
{
	auto p = reinterpret_cast<const uint8_t*>(current) + offset;
	return reinterpret_cast<T>(p);
}

template <typename T> static T at_offset(void* current, int offset)
{
	auto p = reinterpret_cast<uint8_t*>(current) + offset;
	return reinterpret_cast<T>(p);
}

// First object located immediately after start marker in image
constexpr size_t FWFS_BASE_OFFSET{sizeof(uint32_t)};

// Images start with a single word to identify this as a Firmware Filesystem image
constexpr uint32_t FWFILESYS_START_MARKER{0x53465746}; // "FWFS"

// Image end marker (reverse of magic)
constexpr uint32_t FWFILESYS_END_MARKER{0x46574653}; // "SFWF"

// Everything in this header must be portable (at least, with other little-endian systems) so byte-align and pack it
#pragma pack(1)

/** @brief Object type identifiers
 *  @note id is followed by the content size, in either 1, 2 or 3 bytes.
 *  All references have 1-byte size
 *  Everything from Data8 and below has 1-byte size
 *  Data24 uses 3-byte size
 *  Everything else uses 2 byte size
 */
#define FWFS_OBJTYPE_MAP(XX)                                                                                           \
	XX(0, End, "The system image footer")                                                                              \
	XX(1, Data8, "Data, max 255 bytes")                                                                                \
	XX(2, ID32, "32-bit identifier")                                                                                   \
	XX(3, ObjAttr, "Object attributes")                                                                                \
	XX(4, Compression, "Compression descriptor")                                                                       \
	XX(5, ReadACE, "minimum UserRole for read access")                                                                 \
	XX(6, WriteACE, "minimum UserRole for write access")                                                               \
	XX(7, VolumeIndex, "Volume index number")                                                                          \
	XX(8, Md5Hash, "MD5 Hash Value")                                                                                   \
	XX(9, UserAttribute, "User Attribute")                                                                             \
	XX(32, Data16, "Data, max 64K - 1")                                                                                \
	XX(33, Volume, "Volume, top-level container object")                                                               \
	XX(34, MountPoint, "Root for another filesystem")                                                                  \
	XX(35, Directory, "Directory entry")                                                                               \
	XX(36, File, "File entry")                                                                                         \
	XX(64, Data24, "Data, max 16M - 1")

/** @brief Object structure
 *  @note all objects conform to this structure. Only the first word (4 bytes) are required to
 *  nagivate the file system.
 *  All objects have an 8, 16 or 24-bit size field. Content is always immediately after this field.
 *  Reference objects are always 8-bit sized.
 */
struct Object {
	uint8_t typeData; ///< Stored type plus flag

	/**
	 * @brief Object identifier (offset from start of image)
	 */
	using ID = uint32_t;

	enum class Type {
#define XX(value, tag, text) tag = value,
		FWFS_OBJTYPE_MAP(XX)
#undef XX
	};

	// Top bit of object type set to indicate a reference
	static constexpr uint8_t FWOBT_REF{0x80};

	/**
	 * @brief Object attributes
	 * @note these are bit values
	 */
	enum class Attribute {
		/**
		 * @brief Object should not be modified or deleted
		 */
		ReadOnly,
		/** @brief Object modified flag
		 *  @note Object has been changed on disk. Typically used by backup applications
		 */
		Archive,
		/**
		 * @brief Object data is encrypted
		 * 
		 * This is just a hint. Applications will typically provide additional user metadata
		 * to provide any additional information required for decryption.
		 */
		Encrypted,
		//
		MAX
	};

	using Attributes = BitSet<uint8_t, Attribute, size_t(Attribute::MAX)>;

	Type type() const
	{
		return static_cast<Type>(typeData & 0x7f);
	}

	bool isRef() const
	{
		return (typeData & FWOBT_REF) != 0;
	}

	uint32_t getRef() const
	{
		if(!isRef()) {
			return 0;
		}
		uint32_t id{0};
		memcpy(&id, &data8.ref.packedOffset, data8.contentSize());
		return id;
	}

	bool isNamed() const
	{
		return type() >= Type::Volume && type() < Type::Data24;
	}

	bool isData() const
	{
		return type() == Type::Data8 || type() == Type::Data16 || type() == Type::Data24;
	}

	bool isDir() const
	{
		return type() == Type::Directory;
	}

	bool isMountPoint() const
	{
		return type() == Type::MountPoint;
	}

	union {
		// Data8 - 8-bit size, max. 255 bytes
		struct {
			uint8_t _contentSize;
			// uint8_t data[];

			unsigned contentOffset() const
			{
				return offsetof(Object, data8) + 1;
			}

			unsigned contentSize() const
			{
				return _contentSize;
			}

			union {
				// An object reference: the contents are stored externally (on the same volume)
				struct {
					uint32_t packedOffset; // 1-4 byte offset
				} ref;

				// ID32
				struct {
					uint32_t value; ///< 32-bit identifier, e.g. volume ID
				} id32;

				// Object attributes
				struct {
					uint8_t attr; // Attributes
				} objectAttributes;

				// Compression descriptor
				Compression compression;

				// ReadACE, WriteACE
				struct {
					UserRole role;
				} ace;

				// Identifies a volume index, contained in a mount point
				struct {
					uint8_t index;
				} volumeIndex;

				// User attribute
				struct {
					uint8_t tagValue;
					// uint8_t[] data;
				} userAttribute;

				// END - immediately followed by end marker
				struct {
					uint32_t checksum;
				} end;
			};
		} data8;

		// Data16 - 16-bit size, max. (64KB - 1)
		struct {
			uint16_t _contentSize;
			// uint8_t data[];

			unsigned contentOffset() const
			{
				return offsetof(Object, data16) + 2;
			}

			uint32_t contentSize() const
			{
				return _contentSize;
			}

			uint32_t size() const
			{
				return contentOffset() + contentSize();
			}

			union {
				/*
				 * named object
				 *
				 * child objects can be contained or referenced (flag in _id)
				 * object attributes are optional so can be another object/attribute
				 */
				struct {
					uint8_t namelen; ///< Length of object name
					TimeStamp mtime; // Object modification time
					// char name[namelen];
					// Object children[];

					// Offset to object name relative to content start
					unsigned nameOffset() const
					{
						return sizeof(Object::data16.named);
					}

					// Offset to start of child object table
					unsigned childTableOffset() const
					{
						return nameOffset() + namelen;
					}

				} named;
			};
		} data16;

		// Data24 - 24-bit size, max. (16MB - 1)
		struct {
			uint16_t _contentSize;	///< Object size (excluding this header)
			uint8_t _contentSizeHigh; ///< Allows data up to 16MByte
			// uint8_t data[];

			size_t contentOffset() const
			{
				return offsetof(Object, data24) + 3;
			}

			uint32_t contentSize() const
			{
				return (_contentSizeHigh << 16) | _contentSize;
			}

			void setContentSize(uint32_t value)
			{
				_contentSize = value & 0xffff;
				_contentSizeHigh = value >> 16;
			}

			uint32_t size() const
			{
				return contentOffset() + contentSize();
			}
		} data24;
	};

	/** @brief return offset to start of object content
	 */
	size_t contentOffset() const
	{
		if(isRef() || (type() < Type::Data16)) {
			return data8.contentOffset();
		} else if(type() < Type::Data24) {
			return data16.contentOffset();
		} else {
			return data24.contentOffset();
		}
	}

	/** @brief return size of object content, excluding header and size fields
	 *  @retval size or error code
	 *  @note must check return error code
	 */
	uint32_t contentSize() const
	{
		if(isRef() || (type() < Type::Data16)) {
			return data8.contentSize();
		} else if(type() < Type::Data24) {
			return data16.contentSize();
		} else {
			return data24.contentSize();
		}
	}

	uint32_t childTableOffset() const
	{
		assert(isNamed());
		return data16.contentOffset() + data16.named.childTableOffset();
	}

	uint32_t childTableSize() const
	{
		assert(isNamed());
		int size = data16.contentSize() - data16.named.childTableOffset();
		return (size <= 0) ? 0 : size;
	}

	/** @brief total size this object occupies in the image
	 *  @retval size or error code
	 */
	uint32_t size() const
	{
		return contentOffset() + contentSize();
	}
};

static_assert(sizeof(Object) == 8, "Object alignment wrong!");

#pragma pack()

/**
 * @brief FWFS Object Descriptor
 */
struct FWObjDesc {
	Object::ID id; ///< location
	Object obj{};  ///< The object structure

	FWObjDesc(Object::ID objId = 0) : id(objId)
	{
	}

	uint32_t offset() const
	{
		return id;
	}

	uint32_t nextOffset() const
	{
		return offset() + obj.size();
	}

	// Move to next object location
	void next()
	{
		id = nextOffset();
	}

	uint32_t contentOffset() const
	{
		return offset() + obj.contentOffset();
	}

	uint32_t childTableOffset() const
	{
		return offset() + obj.childTableOffset();
	}
};

FileAttributes getFileAttributes(Object::Attributes objattr);
Object::Attributes getObjectAttributes(FileAttributes fileAttr);

} // namespace FWFS
} // namespace IFS

/**
 * @brief Get descriptive String for an object type
 */
String toString(IFS::FWFS::Object::Type obt);
