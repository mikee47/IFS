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
   This is the same Sming filesystem API we're all used to, with a few minor changes and a number of additions.
   File systems are implemented using the IFileSystem virtual class.

Firmware FileSystem (FWFS)
   Files, directories and metadata are all stored as objects in an Object Store, which can be SPIFFS or on a FWRO (Firmware, Read-Only) image.
   FWRO images are compact, fast and use less RAM than SPIFFS.
   To support read/write data SPIFFS can be mounted in a sub-directory.

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


Various IFS implementations are provided:

:cpp:class:`IFS::FWFS::FileSystem`
   Firmware Filesystem. It is designed to support all features  of IFS, whereas other filesystems
   may only use a subset.

:cpp:class:`IFS::SPIFFS::FileSystem`
   SPIFFS filesystem. The metadata feature is used to store extended file attributes.

:cpp:class:`IFS::HYFS::FileSystem`
   Hybrid filesystem. Uses FWFS as the root filesystem, with SPIFFS 'layered' on top.
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
   Files have a standard set of attribute flags plus modification time and simple role-based access control list (ACL)

Directories
   Fully supported, and can be enumerated with associated file information using a standard opendir/readdir/closedir function set.

User metadata
   Supported for application use

:cpp:class:`IFS::IFileSystem`
   The virtual base class which all installable filesystems implement.
   Class methods are very similar to SPIFFS (which is POSIX-like).
   A single FileStat structure is used both for reading directory entries and for regular fileStat operations.
   This differs from SPIFFS which uses different structures, however the IFS implementation conceals this.

Filesystem API
   The Sming FileSystem functions are now wrappers around a single IFileSystem instance, which is provided by the application.

Streaming classes
   Sming provides IFS implementations for these so they can be constructed on any filesystem, not just the main (global) one.

Dynamic loading
   File systems may be loaded/created and unloaded/destroyed at runtime

Multiple filesystems
   Applications may use any supported filesystem, or write their own, or use any combination of existing filesystems to meet requirements.
   The API is the same.


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


Objects
~~~~~~~

FWFS goes further than a simple read-write system.
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


Configuration filesystem
------------------------

@todo

If an application only requires write access for configuration files, SPIFFS is overkill.
These files would be updated very infrequently, so wear-levelling would be un-necessary.
The names and number of files would probably also be known at build time, and an individual file could be limited to a fixed size,
for example one or two flash sectors. A ConfigFileSystem implementation would not need to support file creation or deletion.

Such a system would require almost no static RAM allocation and code size would be tiny.

.. note::

   The ESP-IDF has a mechanism for flash-based configuration space via the ``NVS`` component.
   It is robust and flexible but uses a signficant amount of RAM for buffering which may preclude
   its use with the ESP8266.


Code dependencies
-----------------

Written initially for Sming, the library is portable to other systems.

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


API
---

.. doxygennamespace:: IFS
   :members:
