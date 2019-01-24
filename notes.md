

Accessing communications devices via the file system could also be done, just like linux. The IO control system would do the work but the file system would provide the standard read/write interface. Operations would be asynchronous so I/O accesses would return a pending code. Need to think about how callbacks would be defined. At present a single callback is lodged with the device manager, no reason that can't continue. The request object is the thing that we'd get the result data from. It would not be a stream; the callback would be invoked one or more times for a single call. We'd normally provide a buffer to the read/write method but for asynchronous comms we could forget about that so the callback returns a reference to the device's own internal buffer. Both methods would work, just a flag to indicate buffered or unbuffered transactions.

Using the IO control mechanism, we'd have an IO controller for raw flash and another for SPIFFS. Additional controllers would be handle for native filing systems for debug and tool builds. Each filing system instance would be a CIODevice and requests to those devices would handle the read and write commands.

File handles. SPIFFS uses int16_t whereas FW files are a memory pointer, base flash address is 0x4020000. If we use int32_t as generic file handle we have:

   f < 0                         File errors
   0 < f <= 0x7fff               SPIFF handle
   0x7fff < 0x4020000            invalid
   0x40200000 <= f < 0x40600000  FW handle (range approximate)
   f >= 0x40600000               invalid

Alternatively we can stick with 16 bits use it as an index into an open file object table.

## Memory usage

Heap usage reports 384 bytes used by FirmwareFileSystem.

FirmwareFileSystem + VMT
4	IFSMedia* _media = nullptr;
20	FWFileSysHeader _sys = {};
320	FWFileDesc _fds[FWFILE_MAX_FDS];	// 20 * 16
(344)

IFSFlashMedia + VMT
8	IFSMedia
4	uint32_t _startAddress;
(12)

IFSMedia + VMT
4	uint32_t _size;
1	FSMediaAttributes _attr;
(8)

FileMeta
4	time_t mtime;
1	FileAttr attr;
1	uint8_t reserved;
2	FileACL acl;
(8)

FWFileDesc
12	FWFileHeader header;
4	uint32_t imageOffset;
4	uint32_t fileOffset;
(20)
