# Filesystem builder

## Overview

This tool builds a Firmware Filesystem (FWFS) image. The image is intended to be linked into the firmware, however it can also be converted into an image for any other supported filesystem using the fscopy tool.

Support is included for JSON/js minification, GZIP compression, access control and directories. Directories are handled by using the full (relative) path name in the image. There is no restriction on path length.

A configuration file is used to drive the script. The output is a compact image which can be linked into firmware. Multiple images could be used if required.

## Usage

The builder requires a valid configuration file specified on the command line. See `fsbuild.ini` for an example.

The generated image file is intended to be linked into the firmware as a blob. Typically this will be specified as part of the system makefile.

For debugging purposes a copy of the generated files can be written into another location.

The builder prints a file listing on completion, which you should check carefully to ensure file permissions, attributes, etc. are as expected.

Type `fsbuild.py -h` for command line help.

## Configuration

This section describes the format of the input configuration file.

### `[config]`

`volumeName` specifies a label for the volume

`volumeID` is a 32-bit 'tag' for the volume

### `[source]`

Source files may be read from any location.

Paths are relative to the location of the configuration file.

Absolute paths may be specified.

Environment variables are specified using `{VARNAME}` syntax, and will be expanded.

File modification time is read from the file itself.

Path mappings are specified in `source=target` pairs, for example:

	/ = files
	
Copies the contents of the `files` directory into the root of the image. All sub-directories are included.

	readme.md = ${SMING_HOME}/third-party/IFS/readme.md
	
Puts the `readme.md` file in the root of the image.

### `[rules]`

Specify metadata rules in this section. The general format is:

_file mask_: _rules_

Rules are expressed as _attribute=value_ pairs:

* *read=UserRole* Set minimum `UserRole` required to read the specified file(s)
* * write=UserRole* Set minimum `UserRole` required to write the specified file(s)
* *readonly=true/false* When set, indicates file should not be overwriteable
* *compress=none/gzip* Specifies compression to apply to file(s)

## Compacting

Javascript files are minified, and JSON files are re-written in compact form. This is done irrespective of whether any compression is subsequently applied.

## Compression

GZIP compression is attempted for those files indicate by config rules. If this does not result in a size reduction then the file is left uncompressed. The file attributes indicate if compression has been applied; the file name is not changed.

Compression should not be used for any files which the firmware needs to process internally, as this would require GZIP decompression in the firmware. For the ESP8266 this is not practical, however other platforms may consider this useful.

## Access Control

A simple role-based Access Control List is implemented consisting of two entries, one for read access and another for write. Each entry specifies the minimum user role required. See `access.py` for available `UserRole` values.

Note that the file system does not _enforce_ access control; that is left to the application to deal with.

