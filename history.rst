Installable File System
~~~~~~~~~~~~~~~~~~~~~~~

Control Operations
~~~~~~~~~~~~~~~~~~

To keep the API as simple as possible we should only define `IFileSystem` methods for calls we'd normally expect to find in a POSIX-style filing system API.

.. note::

   A `control` method was added for non-standard calls, however additional methods which aren't part of POSIX have been added to the IFileSystem API.
   The implementations for these would be in separate methods anyway, so accessing them via `control` just makes the code more complicated
   and error prone since we lose strong type checks.

The GNU C library includes `fcntl` to perform various miscellaneous activities on files. We might define such a function to handle these sorts of things:

-  getinfo
-  setacl
-  setattr
-  settime
-  check
-  isfile

The method would take the general form `fcntl(file_t file, int command, ...)`

Note that `ioctl` is a more generic function which applies to any type of device driver. It doesn't really apply here.

So these are the filesystem methods defined:

FileSystem
   -  mount
   -  getinfo
   -  geterrortext

File
   -  stat
   -  fstat
   -  open
   -  fopen
   -  close
   -  read
   -  write
   -  lseek
   -  eof
   -  tell
   -  truncate
   -  flush
   -  fnctl

Directory
-  opendir
-  readdir
-  closedir
