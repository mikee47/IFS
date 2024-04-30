Installable File System
=======================

Created for Sming Framework Project August 2018 by mikee47

I struggled to find anything like this for embedded systems. Probably didn't look hard enough, but it seemed like a fun thing to do so here we are.

The term 'IFS' came about because of the 'I' naming convention for virtual 'Interface' classes, hence IFileSystem.
The term 'installable' is entirely appropriate because IFS allows file systems to be loaded and unloaded dynamically.
Or maybe I just nicked the term from Microsoft :-)

Overview
--------

IFS is written in C++ and has these core components:

FileSystem API
   File systems are implemented using the :cpp:class:`IFS::IFileSystem` virtual class.
   This is, in essence, a single large 'function table' you will see in regular filesystem implementations.

   Class methods are similar to SPIFFS (which is POSIX-like).

   .. note::

      A single :cpp:class:`Stat` structure is used both for reading directory entries and for regular :cpp:func:`fileStat` operations.

      This differs from regular file APIs but is intended to simplify operation.

   Applications will typically use :cpp:class:`IFS::FileSystem` instead, which adds additional methods and
   overloads such as `String` parameter support. This used to implement the standard 'flat' Sming filesystem API,
   with a few minor changes and a number of additions.

   Two wrapper clases (:cpp:class:`IFS::File` and :cpp:class:`IFS::Directory`) are provided for applications
   to manage access to files and folders.

Firmware FileSystem (FWFS)
   Files, directories and metadata are all stored as objects in read-only image.
   FWFS images are compact, fast to access and use very little RAM (approx. 240 bytes for file descriptors, etc.)

   To support read/write data a writeable filesystem can be mounted in a sub-directory.

   A python tool ``fsbuild`` is used to build an FWFS image from user files. See :doc:`tools/fsbuild/README`.

   This is integrated into the build system using the ``fwfs-build`` target for the partition. Example :ref:`hardware_config` fragment:

   .. code-block:: json

      "partitions": {
         "fwfs1": {
            "address": "0x280000",
            "size": "0x60000",
            "type": "data",
            "subtype": "fwfs",
            "filename": "out/fwfs1.bin",
            "build": {
                "target": "fwfs-build",   // To build a FWFS image
                "config": "fsimage.fwfs"  // Configuration for the image
            }
         }
      }

   Sming provides the :sample:`Basic_IFS` sample application which gives a worked example of this.

   Simple filesystem definitions can be defined in-situ, rather than using external file:

   .. code-block:: json

      "build": {
            "target": "fwfs-build",   // To build a FWFS image
            "config": {
               "name": "Simple filesystem",
               "source": {
                  "/": "files"
               }
            }
      }



The following basic IFS implementations are provided in this library:

:cpp:class:`IFS::FWFS::FileSystem`
   Firmware Filesystem. It is designed to support all features  of IFS, whereas other filesystems
   may only use a subset.

:cpp:class:`IFS::HYFS::FileSystem`
   Hybrid filesystem. Uses FWFS as the read-only root filesystem, with a writeable filesystem 'layered' on top.

   When a file is opened for writing it is transparently copied to the SPIFFS partition so it can be updated.
   Wiping the SPIFFS partition reverts the filesystem to its original state.

   Note that files marked as 'read-only' on the FWFS system are blocked from this behaviour.

:cpp:class:`IFS::Host::FileSystem`
   For Host architecture this allows access to the Linux/Windows host filesystem.

:cpp:class:`IFS::Gdb::FileSystem`
   When running under a debugger this allows access to the Host filesystem.
   (Currently only works for ESP8266.)


IFS (and FWFS) has the following features:

Attributes
   Files have a standard set of attribute flags plus modification time and simple role-based access control list (ACL).

Directories
   Fully supported, and can be enumerated with associated file information using a standard opendir/readdir/closedir function set.

User metadata
   Supported for application use. The API for this is loosely based on Linux extended attributes (non-POSIX).
   Attributes are small chunks of data attached to files and directories, each identified by a numeric :cpp:enum:`IFS::AttributeTag`.

Filesystem API
   The Sming FileSystem functions are now wrappers around a single IFileSystem instance, which is provided by the application.

Streaming classes
   Sming provides IFS implementations for these so they can be constructed on any filesystem, not just the main (global) one.

Dynamic loading
   File systems may be loaded/created and unloaded/destroyed at runtime

Multiple filesystems
   Applications may use any supported filesystem, or write their own, or use any combination of existing filesystems to meet requirements.
   The API is the same.

Mount points
   FWFS is designed for use as a read-only root filing system, and supports mounting other filesystems in special directories.



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
Images can be mounted in separate partitions, linked into the program image itself or stored as files
within another filesystem.

.. note::

   This behaviour is supported by partitions (see :component:`Storage`) using custom :cpp:class:`Storage::Device` objects.


Redirection
~~~~~~~~~~~

FWFS incorporates a redirector. This works by creating a mount point (a named object), which looks like an empty directory.
When accessed, this get redirected to the root of another filesystem.
The maximum number of mount points is fixed at compile time, but file systems can be mounted and dismounted at any time.

Mount points are identified explicitly in the build configuration file:

.. code-block:: json

   "mountpoints": {
      "path/to/use/spiffs": 0,
      "path/to/use/littlefs": 1
   }

The filesystem builder creates the MountPoint objects and tags them with the given volume indices.
For example, the directory "path/to/use/littlefs" is attached to volume index #0.

.. note::
   
   Unlike other filesystems you cannot use a regular directory as a mountpoint.
   To change the name of a mountpoint requires the filesystem image to be re-built and re-flashed.

Applications use the :func:`IFileSystem::setVolume` method to install the actual filesystem.


Streaming backup/archive support
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :cpp:class:`IFS::FWFS::ArchiveStream` class can be used to generate streaming filesystem backups
from any supported filesystem. The archive files are in FWFS format.

Here are some examples of how it can be used:

-  Stream filesystem (or directory) images directly to remote servers
-  Make local filesystem backups
-  Compact log files which don't change much (think of ZIP files - just needs a compression plugin)
-  Backup entire filesystem a local file, an empty partition, etc.
-  Defragment/compact or repair a damaged filesystem by re-formatting then restoring from backup

The archiver has some additional features:

-  Specify whether to archive an entire filesystem or start from a specific directory
-  Specify whether to follow links (e.g. other filesystems in mountpoints) or not
-  Exclude any file or directory via custom callback (or by overriding methods)
-  Perform custom file data encoding such as compression or encryption via callbacks
-  Add additional metadata to files (comments, encryption codes, etc.)

See the :sample:`Basic_IFS` sample for 


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


Configuration filesystem
------------------------

If an application only requires write access for configuration files, SPIFFS is overkill.
These files would be updated very infrequently, so wear-levelling would be un-necessary.
The names and number of files would probably also be known at build time, and an individual file could be limited to a fixed size,
for example one or two flash sectors. A ConfigFileSystem implementation would not need to support file creation or deletion.

Such a system would require almost no static RAM allocation and code size would be tiny.


However, the :library:`LittleFS` has excellent metadata support and is ideal for storing configuration information.
This can be done using :cpp:method:`IFS::FileSystem::setUserAttribute` and read using :cpp:method:`IFS::FileSystem::getUserAttribute`
or :cpp:method:`IFS::FileSystem::enumAttributes`.


.. note::

   The ESP-IDF has a mechanism for flash-based configuration space via the ``NVS`` component.
   It is robust and flexible but uses a significant amount of RAM for buffering which may preclude
   its use with the ESP8266.



FWFS Objects
~~~~~~~~~~~~

All files, directories and associated information elements are stored as 'objects'.
Files and directories are 'named' objects, which may contain other objects either directly or as references.
Small objects (255 bytes or less) are stored directly, larger ones get their own file. Maximum object size is 16Mbytes.

File content is stored in un-named data objects.
A named object can have any number of these and will be treated as a single entity for read/write operations.
File 'fragments' do not need to be contiguous, and are reassembled during read operations.

**Named** objects can be enumerated using :cpp:func:`IFS::IFileSystem::readdir()`.
Internally, FWFS uses handles to access any named object.
Handles are allocated from a static pool to avoid excessive dynamic (heap) allocation.
Users can attach their own data to any named object using custom object types.

The filesystem layout is displayed during initial mount if this library is built with :envvar:`DEBUG_VERBOSE_LEVEL` = 3.

Why FWFS?
---------

There are many existing candidates for a read-only system, so why do we need another one? Here are some reasons:

-  SPIFFS and LittleFS could be used in read-only mode but they are not designed for space-efficiency. Images are therefore
   larger than necessary, sometimes considerably larger. This is also true of other such filesystems designed for Linux, etc.

   FWFS is designed to produce the smallest possible images to conserve limited flash storage.
   It therefore has a high effective capacity, i.e. you can put a lot more in there than with other filesystems.

-  With ROMFS, for example, information is laid out with headers first, followed by data.
   The root directory and volume information are at the front.

   FWFS works in reverse by writing out file contents first, then file headers and then directory records.
   The root directory comes at the end, followed by the volume information record.
   This allows images to be created as a stream because directory records can be efficiently constructed in RAM
   as each file or subdirectory record is written out. This keeps memory usage low.

   In addition, checksums and additional metadata can be created while file data is written out. This could be required for
   compressing or encrypting the contents, or for error tolerance. For example, if corruption is encountered whilst reading
   file contents this can be noted in the metadata which is written out afterwards.

   Filesystem images are therefore generated in a single pass, with each file or directory only read once.

-  Standard attribute support not well-suited to embedded microsystems.

   The small set of standard metadata defined by IFS is designed to solve specific problems with typical IOT applications.

   
Code dependencies
-----------------

Written initially for Sming, the library should be fairly portable to other systems.

No definitions from SPIFFS or other modules should be used in the public interface; such dependencies should be managed internally.

Applications should avoid using filesystem-dependent calls, structures or error codes.
Such code, if necessary, should be placed into a separate module.


Implementation details
----------------------

The traditional way to implement installable filing systems is using function tables, such as you'll see in Linux.
One reason is because the Linux kernel is written in C, not C++. For Sming, a virtual class seems the obvious choice, however there are some pros and cons.

VMT
   Advantages
      -  Compiler ensures correct ordering of methods, parameter type checking
      -  Simpler coding
      -  Extending and overriding is natural

Function table
   Advantages
      -  Portable to C applications (although with some fudging so are VMTs).

   Disadvantages
      -  Care required to keep function order and parameters correct. Very likely we'd use a bunch of macros to deal with this.

Macros

   We could #define the active filing system name which the FileSystem functions would map to the appropriate call.
   For example, fileOpen would get mapped to SPIFlashFileSystem_open().
   We need to provide macros for defining file system functions.

   Advantages
      -  Fast

   Disadvantages
      -  Complicated
      -  Prone to bugs
      -  Not C++


Configuration variables
-----------------------

.. envvar:: FWFS_DEBUG

   default: 0

   Set to 1 to enable more detailed debugging information.


.. envvar:: ENABLE_FILE_SIZE64

   default: disabled

   Set to 1 to enable support for 64-bit files.
   This requires :envvar:`ENABLE_STORAGE_SIZE64` to be set.


API
---

.. doxygennamespace:: IFS
   :members:
