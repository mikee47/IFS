Installable File System
=======================

FWFS object indices
~~~~~~~~~~~~~~~~~~~

OK, so we can have resident and non-resident objects, but at present we can't have named objects inside other named objects. If fsbuild assigns indices to all named objects, resident or otherwise, then we'll have gaps in the top level index order. The cache isn't doing much so forget about that.

So, for example, we're looking for object #25. We find 21, 22, 29 so where are 23-28? They must be inside #22. So we go back to #22 and scan inside that. We need to repeat this recursively.


Inheritable objects
~~~~~~~~~~~~~~~~~~~

OK, so I've made ACEs inheritable. Find for image building, but how do we track it in the file system?
Example:
  FileHandle file = open("index.html")

What about grandparents? This would indicate the owning object, but if we're dealing with a file then a single level of inheritance may be sufficient; we set ACL on the directory which becomes the default for all immediate children. Thus every file doesn't need an ACL, but the sub-folders do. Not sure about this...

The file descriptor doesn't know anything about an object's parents, but we could keep a list of object indices for each object in a path. If we restrict path depth to say, 4 levels that's 8 bytes we'd need to add to a descriptor. Alternatively we store the ACL in the descriptor itself, so it gets built up as the path is parsed. That's 2 bytes. Handy, we've got 2 bytes available :-)
The whole inheritance thing was attempting to reduce the number of objects. If we're going to store the ACE inside the object then that's not such a problem. Inheritance is still a good idea for security though.

For now, probably best to forget about inheritance. Let fsbuild deal with that.


FWFS Image creation
~~~~~~~~~~~~~~~~~~~

Add image writing capability so allow streaming archives to be created. Use cases include backup, defragmentation or rebuilding of a SPIFFS volume by archiving files into an area of flash, reformatting the SPIFFS volume then writing the content back in.


Directories vs. files
~~~~~~~~~~~~~~~~~~~~~

These are both named objects. We use a separate object for this, dynamically allocated. The alternative is to allocate a file descriptor for directory parsing. In this model, method would behave as follows:
  * fopen / open - returns a handle to any named object, including directories, for accessing the child data object(s)
  * opendir - returns a handle to any named object, including files, for accessing the child table entries
  * read - gets content as determined by the openXXX method used to create the handle
  * readdir - interprets the data as child object indices and returns information for any named objects found. If an object index is invalid, returns an error, but if it's not a named object type then it just ignores it. As such there's not a lot of validation here, but it won't crash anything.

Implementation

   For directory enumeration we have FileDir:
      magic - integrity
      dirObj - the directory object descriptor
      index - position of next object to return in children table
   
   A file descriptor has these things for reading file content (data objects):
      offset - location of the named object
      index - index of the named object
      extent - offset and length of the data object content

To use this for directories we can just set the extent to the child table. We also need a flag to indicate what the extent represents. Whilst using read() on this data might be useful, and generally not problematic, readdir() needs a valid list of child indices; Having said that, if the user wanted to keep an efficient list of file references then instead of filenames they'd just use the file IDs, aka. object indices. If these indices were stored in a file, then we could legitimately use open() on the file, then call readdir() to interpret those file indices. In that situation.

So as it turns out we don't need to do anything special with file descriptors, it's all down to the allocateDescriptor() method to determine what to put in the extent.

metadata (SPIFFS)
~~~~~~~~~~~~~~~~~

Add support for user metadata area. We'd reserve the first section for internal use but both API and builder would support adding arbitrary user information.

SPIFFS::FileSystem uses a cache for metadata. fileSetACL, fileSetAttr and fileSetTime all update the metadata. In particular, the file time is updated on every write so that must be cached. It is unclear what user metadata might be used for, but caching it makes it more flexible. For example, an application might use it as a scratchpad while a file is open, then discard it (by calling fileSetAttr to clear the dirty bit) before closing the file. That way it never hits the disk. Alternatively, we could save a bit of RAM by never caching the user metadata. When fileUpdateMeta() is called it calls SPIFFS directly, also flushing through any cached internal metadata. This could be selectable by a #define, of particular use if the application requires a large metadata area.

LittleFS is a better choice where flexible user metadata is required.

Asynchronous I/O
~~~~~~~~~~~~~~~~

If files are located on slow devices then a read/write request will hog the system. Network devices are already handled by Sming/LWIP using streams and callbacks. If an application wished to process file data as it arrives, it can use a custom stream object to do the work. Network devices are therefore already well catered for.

What about locally-attached hardware devices? The SDCard library uses software SPI (the ESP8266 hardware SPI is used for external Flash memory) so little to be done.

Serial port transfers buffered in hardware. Storage devices aren't generally connected via RS232 though so unlikely to fit within a filesystem context.

It seems unlikely therefore that the file system itself will need to handle asynchronous transfers. It'll just be for immediate storage.


Path virtualisation
~~~~~~~~~~~~~~~~~~~

What if we could access files using paths like 'file://index.html' or 'localhost://index.html'. An OS would redirect these requests to the local filesystem, so could we. The advantage is clear: it provides a single, simple way to get content from pretty much anywhere. A Sming URI redirector could handle requests for:

	http://
	ftp://
	file://
	mqtt://

etc.

Sming has redirectors for HTTP using HttpResource objects. 

FWFS extension
~~~~~~~~~~~~~~

By inheriting from the FWFS::FileSystem class we should be able to add folder redirection for other filesystems. For example, mounting a SPIFFS filesystem under 'config/'. This would be simpler than the Hybrid implementation. It would also improve file open performance; tests on real hardware show that fileOpen operations take around 20x longer on SPIFFS than on FWFS (e.g. 5ms vs 200us) [31/1/19 not sure how current these figures are], so HFS is similarly hindered. There's no significant difference for read operations.

File system construction
~~~~~~~~~~~~~~~~~~~~~~~~

Firmware Filesystem images are build using a python script. Support is included for JSON/js minification, GZIP compression, access control and directories. A configuration file is used to drive the script. The output is a compact image which can be linked into firmware. Multiple images may be used.

One way to create a similar SPIFFS image is to build the FWFS image first, then copy it into a SPIFFS filesystem. This will be the role of the **fscopy** program, written in C++ and using the IFS API with SPIFFS.

Image size
   Specify directly, or the amount of free space required; the program will then calculate the appropriate image size.

Metadata size
   How much space to allocate for user metadata.

Maximum filename length
   Bear in mind SPIFFS doesn't implement directories so, like FWFS, the full (relative) path needs to be accounted for. If this limit is exceeded the program will fail.	

We need a tool (in C++) which python can use to actually fabricate the images for any supported filing system.

FAT support
~~~~~~~~~~~

Add wrapper for FAT file system. May need some more methods in IFileSystem

Filename pattern matching
~~~~~~~~~~~~~~~~~~~~~~~~~

Regex-style file searches ?

File Attributes
~~~~~~~~~~~~~~~

Enforce the READONLY flag bit by failing open or remove calls.
