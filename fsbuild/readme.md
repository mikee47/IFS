# Filesystem builder

## Overview

This tool builds a Firmware Filesystem (FWFS) image for the Sming Installable Filesystem (IFS). The image is intended to be linked into the firmware, however it can also be converted into an image for any other supported filesystem using the fscopy tool.

Support is included for JSON/js minification, GZIP compression, access control and directories. Directories are handled by using the full (relative) path name in the image. There is no restriction on path length.

A configuration file is used to drive the script. The output is a compact image which can be linked into firmware. Multiple images could be used if required.

## Usage

This script should be run from the development directory. Parameters are read from the fsbuild.ini file located in the same folder. An example is provided.

Source files should be placed into a sub-folder; default is 'Files'. Sub-directories are fully supported.

The generated image file is intended to be linked into the firmware as a blob; default is 'out/fwfiles.bin'.

For debugging purposes a copy of the generated files can be written into another location. By default this is disabled, but a suggested folder might be 'out/files'.

## Features

A simple role-based Access Control List is implemented consisting of two entries, one for read access and another for write. Each entry specifies the minimum user role required.

Note that the file system does not _enforce_ access control; that is left to the application to deal with.

Javascript files are minified, and JSON files are re-written in compact form.

The file tree is read in its entirety, including sub-folders. File modification time is read from the file itself. File ACL information and attributes are provided by parsing the [rules] section of the config file. This also specifies which files may be compressed.

## Compression

GZIP compression is attempted for those files indicate by config rules. If this does not result in a size reduction then the file is left uncompressed. The file attributes indicate if compression has been applied; the file name is not changed.

Compression should not be used for any files which the firmware needs to process internally, as this would require GZIP decompression in the firmware. For the ESP8266 this is not practical, however other platforms may consider this useful.
