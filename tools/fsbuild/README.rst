Filesystem builder
==================

.. highlight:: json

Overview
--------

This tool builds a Firmware Filesystem (FWFS) image file.

Support is included for JSON/js minification, GZIP compression, access control and directories. Directories are handled by using the full (relative) path name in the image. There is no restriction on path length.

A JSON configuration file is used to drive the script. The output is a compact image which can be linked into firmware.

A schema for this configuration is provided in ``tools/fsbuild/schema.json``.
The builder checks the configuration against this schema and will fail if it is non-compliant.
Your IDE (e.g. vscode) may also be able to use this to assist editing the configuration.

Usage
-----

The builder requires a valid configuration file specified on the command line: see ``example.fwfs``.

For debugging purposes a copy of the generated files can be written into another location.

In verbose mode the builder prints a full file listing on completion, which you can use to verify file permissions, attributes, etc. are as expected.

Type ``fsbuild.py -h`` for command line help.

Source definitions
------------------

Source files may be read from any location.

Paths are relative to the current working directory where the script is executed from.
Typically this will be the same as the location of the configuration file itself.

Absolute paths may be specified.

Environment variables are specified using ``${VARNAME}`` syntax.

File modification time is read from the file itself.

Path mappings are specified in ``"{source}": "{target}"`` pairs, for example:

	"/": "files"
	
Copies the contents of the ``files`` directory into the root of the image. All sub-directories are included.

	"ifs-readme.rst": "${SMING_HOME}/Components/IFS/README.rst"

Puts the ``README.rst`` file in the root of the image, renaming it as ``ifs-readme.txt``.

Rule definitions
----------------

These are described in the associated schema.

Compacting ('minification')
---------------------------

-	Javascript files are minified
-	JSON files are compacted (comments and surplus whitespace removed)

This is done automatically and before any compression (if requested).

Compression
-----------

GZIP compression is attempted for those files indicate by config rules. If this does not result in a size reduction then the file is left uncompressed. The file attributes indicate if compression has been applied; the file name is not changed.

Compression should not be used for any files which the firmware needs to process internally, as this would require GZIP decompression in the firmware. For the ESP8266 this is not practical, however other platforms may consider this useful.

Access Control
--------------

A simple role-based *Access Control List* (ACL) is supported using two entries (ACEs), one for *read* access and another for *write*.
Each entry specifies the minimum user role required. See :cpp:enum:`IFS::UserRole`.

This information is reported in :cpp:class:`IFS::File::Stat` which the application may use to enforce these permissions.

.. important::

	The file system does **not** enforce access control by itself.
