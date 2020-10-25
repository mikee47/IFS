Installable File System
=======================

Created for Sming Framework Project August 2018 by mikee47

I struggled to find anything like this for embedded systems. Probably didn't look hard enough, but it seemed like a fun thing to do so here we are.

The term 'IFS' came about because of the 'I' naming convention for virtual 'Interface' classes, hence IFileSystem. The term 'installable' is entirely appropriate because IFS allows file systems to be loaded and unloaded dynamically. Or maybe I just nicked the term from Microsoft :-)

Overview
--------

IFS is written in C++ and has these core components:

FileSystem API
   This is the same Sming filesystem API we're all used to, with a few minor changes and a number of additions.
   File systems are implemented using the IFileSystem virtual class.
Media access layer
   Virtualises physical hardware using the IFSMedia class to allow filing systems to be easily mounted on any suitable storage medium.
Firmware FileSystem (FWFS)
   Files, directories and metadata are all stored as objects in an Object Store, which can be SPIFFS or on a FWRO (Firmware, Read-Only) image.
   FWRO images are compact, fast and use less RAM than SPIFFS. To support read/write data SPIFFS can be mounted in a sub-directory.

Two tools are used for manipulating filesystem images:
	
fsbuild
   builds an FWFS image from user files. Written in python and can be easily be integrated into the Sming tool chain. It's driven by a simple .ini file which allows full control over the build.
fscopy
   converts an FWFS image into other formats (e.g. SPIFFS). Written in C++ using the IFS framework.

Two additional IFS implementations are provided:

SpiFlashFileSystem
   A SPIFFS wrapper, using file metadata to support the extended attributes
HybridFileSystem
   Layers SPIFFS over FWFS, devised to allow deployment of a standard setup which can be modified in-situ and reverted to 'factory defaults' by just running a format.

IFS has the following features:

Attributes
   Files have a standard set of attribute flags plus modification time and simple role-based access control list (ACL)
Directories
   Fully supported, and can be enumerated with associated file information using a standard opendir/readdir/closedir function set.
User metadata
   Supported for application use
Filesystem Builder (fsbuild)
   JSON files are output in compact form
   javascript .js files are minified
   Files may be optionally compressed (GZip). The filename remains unchanged, but a file attribute indicates compression state and type.
   This is generally intended for serving files to network clients, which have the resources available for decompression.
IFileSystem
   The virtual base class which all installable filesystems implement.
   Class methods are very similar to SPIFFS (which is POSIX-like).
   A single FileStat structure is used both for reading directory entries and for regular fileStat operations.
   This differs from SPIFFS which uses different structures, however the IFS implementation conceals this.
IFSMedia
   Provides media virtualisation.
Filesystem API
   The Sming FileSystem functions are now wrappers around a single IFileSystem instance, which is provided by the application
Dynamic loading
   File systems may be loaded and unloaded at runtime
Multiple filesystems
   Applications may use any supported filesystem, or write their own, or use any combination of existing filesystems to meet requirements.
   The API is the same.

Note that IFS does not have the concept of 'current' or 'working' directories.

FWFS
----

Many applications require a default, often fixed set of files. The easiest way is just to use SPIFFS.
The problem is that power outages can corrupt a filesystem. For an embedded device that's bad news.
SPIFFS is also a bit overkill if you're just storing configuration data, or it's just for read-only use.

So what do you do if your filesystem gets wiped? Resetting a system back to a functional, default state can be tricky
if the core user interface web files are gone. You could reformat and pull a standard set of files off a server somewhere.
If your storage requirements are minimal, you could link the file data into your firmware as constant data blocks.

That's kind of what FWFS does, but in a more structured and user-friendly way.

FWFS offers a more convenient solution by providing all your default files in a compact, fast, read-only format.
Using IFS, you can have different images and you can store the images in any available part of flash memory.
Or even on a different memory chip (flash or RAM).

Objects
~~~~~~~

FWFS goes further than a simple read-write system.
All files, directories and associated information elements are stored as 'objects'.
Files and directories are 'named' objects, which may contain other objects either directly or as references.
Small objects (255 bytes or less) are stored directly, larger ones get their own file. Maximum object size is 16Mbytes.

File content is stored in un-named data objects.
A named object can have any number of these and will be treated as a single entity for read/write operations.
You might think of this as support for fragmented files.

Named objects can be enumerated using :cpp:func:`IFS::IFileSystem::readdir()`.
Internally, FWFS uses handles to access any named object.
Handles are allocated from a static pool, so no dynamic (heap) allocation is required.
Users can attach their own data to any named object using custom object types.

Object stores
~~~~~~~~~~~~~

FWFS uses a mid-level layer to deal with reading and writing objects.
It is at a slightly higher level than media, which allows efficient use of flash storage for direct read-only access,
or SPIFFS as a read/write layer. Each object store provides access to a single volume.

When implemented on SPIFFS, a named object is stored as a file. So a directory is just another file.
If the directory contains large files, then the corresponding data objects will be stored separately as other SPIFFS files.
Names are not required on the SPIFFS volume as Object IDs are used exclusively.

Be aware, therefore, that any change to an object will involve rewriting the underlying file.
The SPIFFS object store can probably be improved.
For example, as FWFS doesn't require file content to be in a single object, appending to a file can be done by creating
new data objects and appending them to the file object.
SPIFFS will do this sort of thing anyway, so there should be a way to combine the two.

Redirection
~~~~~~~~~~~

FWFS incorporates a redirector. This works by creating a mount point (a named object), which looks like a directory.
When accessed, this get redirected to the root of another object store.
The maximum number of mount points is fixed at compile time, but stores can be mounted and dismounted at any time.

Archival
~~~~~~~~

One possible application for FWFS images is for archiving.
Multiple filesystem images could be stored on a web server and pulled into memory as required.

Images can be generated 'on the fly' as a backup archive using a archive object store; this would support writing only.

Access Control
--------------

This came about because I wanted to secure down my ESP8266 web server applications so that only the basic index.html,
stylesheets and accompanying javascript would be publicly accessibly. Everything else would require user authentication.

I also wanted to prevent certain users from accessing restricted files. Other users would also be able to edit files.
So a simple role-based access control mechanism seemed appropriate.

Access control typically encapsulates two areas:

Authentication
   Is the user who they say they are? Usually performed by validating a username/password combination. 
Authorisation
   What is the user permitted to do?

I'll step aside for a brief word on security. Authentication is the weakest link because it's exposed to public scrutiny.
To avoid compromise authentication **must only** be done over a secured link. That means SSL.

If you have the option it's usually best to put all your smart devices behind a secure proxy.
The raspberry Pi is great for stuff like this. The Pi deals with keeping the public connection secure,
and translates it into a regular HTTP connection for the ESP8266.

If you don't have this option, but you need to connect your ESP8266 to the internet, use the SSL build for Sming.

Having done this, we don't need to worry about encrypting passwords as the SSL layer will do that.
We just need to make sure they're good passwords.

In my applications authentication is done by matching username/password against the user database, stored in a JSON file.
If successful, the session gets a token which appears in every subsequent request. The user database indicates a **User Role**,
one of *public*, *guest*, *user*, *manager* or *admin*.
IFS keeps an 'Access Control List' (ACL) for each file containing two entries (ACE), one for read access and another for write access.
The ACE specifies the *minimum* assigned :cpp:enum:`IFS::UserRole` required for access.

This is probably as much as the filesystem needs to do.
I can't see that file ownership, inherited permissions or more finely-grained access permissions would be required,
but having said that extending this system would probably be fairly straightforward.


IFS Media Objects
-----------------

This was added as a standardised HAL for filing systems to use, if they choose. There are two standard :cpp:class:`IFS::Media` classes:

:cpp:class:`IFS::FlashMedia`
   Provides configurable access to a flash memory region
:cpp:class:`IFS::StdFileMedia`
   Allows tools to be built working directly on file images

FWFS was originally written to operate using memory buffers, so all accesses went through the hardware caching system.
If some files were accessed frequently this might be an advantage; in fact, a Media object could be written to take advantage
of this for certain files. In general, however, using the cache is undesirable because it will degrade code execution performance.

FatFS provides disk_xxx function prototypes in diskio.h which the SDCard library provides.
As a legacy filesystem it's unlikely we'll need FAT on any other media, so a IFS implementation could just wrap both SDCard + FatFS.
It would probably require a little modification to the SDCard/FatFS library to implement a Media object.

The ESP32-S2 devices have USB OTG, so USB-attached drives are something else to consider.

Tools and test applications running on a development platform (Windows/Linux) need a HAL so we can test IFS effectively.

At present IFS is not suitable for slow devices - see discussion of asynchronous I/O.

Hybrid File System
------------------

Presents a 'SPIFFS-over-Firmware' system. A freshly 'formatted' system will present only the firmware files.
When a file is written, deleted or otherwise modified (including metadata) it is transparently copied into SPIFFS.
The original layout is restored using format().

Configuration filesystem
------------------------

@todo

If an application only requires write access for configuration files, SPIFFS is overkill.
These files would be updated very infrequently, so wear-levelling would be un-necessary.
The names and number of files would probably also be known at build time, and an individual file could be limited to a fixed size,
for example one or two flash sectors. A ConfigFileSystem implementation would not need to support file creation or deletion.

Such a system would require almost no static RAM allocation and code size would be tiny.

Note that the ESP-IDF has a mechanism for flash-based configuration space.

Code dependencies
-----------------

Written initially for Sming, the library is portable to other systems.

No definitions from SPIFFS or other modules should be used in the public interface; such dependencies should be managed internally.
This means that for Sming, it would be preferable to incorporate SPIFFS into the IFS library so applications don't 'see' it.

Applications should avoid using filesystem-dependent calls, structures or error codes. Such code, if necessary, should be placed into a separate module.

Sming/Core/FileSystem has been modified to use these IFS but the API remains largely unchanged, although somewhat expanded.
The basic type definitions were moved into this header file. Access functions are now mainly just wrappers around filing system calls.
A single global IFileSystem instance is used.

Applications may still call spiffs_mount() and spiffs_unmount(). These are defined in the services/SpifFS/spiffs_sming module which has also been updated.

This does not depend on SPIFFS or any other filesystem definitions. An IFS implementation provides a wrapper for such a system.

Implementation details
----------------------

The traditional way to implement installable filing systems is using function tables, such as you'll see in Linux.
One reason is because the Linux kernel is written in C, not C++. For Sming, a virtual class seems the obvious choice, however there are some pros and cons.

VMT
   Advantages
      -  Compiler ensures correct ordering of methods, parameter type checking
      -  Simpler coding
   Disadvantages
      -  Uses RAM where we might not need to. All methods for a filing system get linked in even if they're not used. Probably.

Function table
   Advantages
      -  We can place the tables directly into PROGMEM to minimise RAM usage.
      -  Portable to C applications (although with some fudging so are VMTs).

Disadvantages
   Care required to keep function order and parameters correct. Very likely we'd use a bunch of macros to deal with this.

Macros
~~~~~~

We could #define the active filing system name which the FileSystem functions would map to the appropriate call.
For example, fileOpen would get mapped to SPIFlashFileSystem_open(). We need to provide macros for defining file system functions. Complicated.

Function codes
~~~~~~~~~~~~~~

Instead of one method/function per call, we'd have a generic call with a function code, something like ioctl(code, inbuf, insize, outbuf, outsize) for example.
This would allow considerable flexibility in implementing specialised functions.
However, the compiler would be unable to optimise-out unused functions and we would lose much of its help in checking parameters, etc.

The initial implementation used classes for the filesystem, directory and file objects. It got remarkably unwieldy...

In the end, simplicity won over.

NB. We're not going for full-on OS filing systems here. The next step would be something like FreeRTOS which does all this kind of thing.
If only we had the RAM. The little ESP8266 deserves some special attention :-)

