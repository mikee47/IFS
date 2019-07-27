/*
 * IFSObjectStore.h
 *
 *  Created on: 1 Sep 2018
 *      Author: mikee47
 *
 */

#pragma once

#include "ifstypes.h"
#include "FWFileDefs.h"
#include "IFSMedia.h"

/** @brief gives the identity and location of an FWFS object
 */
struct FWObjRef {
	uint32_t offset = 0; ///< Offset from start of image
	FWObjectID id = 0;
	uint8_t handle = 0;	///< SPIFFS object store requires handles
	uint8_t storenum = 0;  ///< Object store number
	uint8_t readCount = 0; ///< For profiling
	uint8_t refCount = 0;  ///< For testing open/close correctness

	FWObjRef()
	{
	}

	FWObjRef(const FWObjRef& parent, FWObjectID objID)
	{
		storenum = parent.storenum;
		id = objID;
	}

	FWObjRef(uint32_t fileid)
	{
		storenum = fileid >> 16;
		id = fileid & 0xffff;
	}

	~FWObjRef()
	{
		assert(refCount == 0);
	}

	uint32_t fileID() const
	{
		return (storenum << 16) | id;
	}
};

/** @brief FWFS Object Descriptor
 */
struct FWObjDesc {
	FWFS_Object obj; ///< The object structure
	FWObjRef ref;	///< location

	FWObjDesc()
	{
	}

	FWObjDesc(const FWObjRef& objRef) : ref(objRef)
	{
	}

	FWObjDesc(uint32_t fileid) : ref(fileid)
	{
	}

	uint32_t nextOffset() const
	{
		return ref.offset + obj.sizeAligned();
	}

	// Move to next object location
	void next()
	{
		ref.offset = nextOffset();
		++ref.id;
	}

	uint32_t contentOffset() const
	{
		return ref.offset + obj.contentOffset();
	}

	uint32_t childTableOffset() const
	{
		return ref.offset + obj.childTableOffset();
	}
};

class IFSObjectStore
{
public:
	virtual ~IFSObjectStore()
	{
	}

	/** @brief called by FWFS
	 *  @retval error code
	 */
	virtual int initialise() = 0;

	/** @brief called by FWFS to confirm successful mount
	 *  @param od last object (the end marker)
	 *  @retval int error code
	 */
	virtual int mounted(const FWObjDesc& od) = 0;

	/** @brief determine if store is mounted
	 *  @retval true if mounted
	 */
	virtual bool isMounted() = 0;

	/** @brief find an object and return a descriptor for it
	 *  @param od IN/OUT: resolved object
	 *  @retval int error code
	 *  @note od.ref must be initialised
	 */
	virtual int open(FWObjDesc& od) = 0;

	/** @brief open a descriptor for a child object
	 *  @param parent
	 *  @param child reference to child, relative to parent
	 *  @param od OUT: resolved object
	 *  @retval int error code
	 */
	virtual int openChild(const FWObjDesc& parent, const FWObjDesc& child, FWObjDesc& od) = 0;

	/** @brief read a root object header
	 *  @param od object descriptor, with offset and ID fields initialised
	 *  @retval error code
	 *  @note this method deals with top-level objects only
	 */
	virtual int readHeader(FWObjDesc& od) = 0;

	/** @brief fetch child object header
	 *  @param parent
	 *  @param child uninitialised child, returns result
	 *  @retval error code
	 *  @note references are not pursued; the caller must handle that
	 *  child.ref refers to position relative to parent
	 *  Implementations must set child.storenum = parent.storenum; other values
	 *  will be meaningless as object stores are unaware of other stores.
	 */
	virtual int readChildHeader(const FWObjDesc& parent, FWObjDesc& child) = 0;

	/** @brief read object content
	 *  @param offset location to start reading, from start of object content
	 *  @param size bytes to read
	 *  @param buffer to store data
	 *  @retval number of bytes read, or error code
	 *  @note must fail if cannot read all requested bytes
	 */
	virtual int readContent(const FWObjDesc& od, uint32_t offset, uint32_t size, void* buffer) = 0;

	/** @brief close an object descriptor
	 *  @param od
	 *  @retval int error code
	 *  @note the implementation should free any allocated resources
	 */
	virtual int close(FWObjDesc& od) = 0;

	virtual IFSMedia* getMedia() = 0;
};
