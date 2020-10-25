#!/usr/bin/env python3
#
# Script to build Firmware Filesystem image
#
# See readme.md for further information
#

import os, json, sys
import util, FWFS, config
from util import _BV
from FWFS import FwObt
from compress import CompressionType
import argparse

parser = argparse.ArgumentParser(description='Firmware Filesystem Builder')
parser.add_argument('-l', '--log', metavar='filename', help='Create build log file, use `-` to print to screen')
parser.add_argument('-f', '--files', metavar='directory', help='Create file layout for inspection')
parser.add_argument('-i', '--input', metavar='filename', required=True, help='Source configuration file')
parser.add_argument('-o', '--output', metavar='filename', required=True, help='Destination image file')
parser.add_argument('-v', '--verbose', action='store_true', help='Show build details')

args = parser.parse_args()

configFile = os.path.expandvars(util.ospath(args.input))
configDir = os.path.dirname(configFile)

# Parse the configuration file
cfg = config.Config(configFile)

# Non-absolute file paths are relative to location of config file
if configDir != "":
    os.chdir(configDir)

img = FWFS.Image(cfg.volumeName(), cfg.volumeID())

outFilePath = args.files
if outFilePath:
    print("Writing copy of generated files to '" + outFilePath + '"')
    util.mkdir(outFilePath)
    util.cleandir(outFilePath)

if args.log:
    if args.log == '-':
        logfile = sys.stdout
    else:
        logfile = open(args.log, 'w')
    fmtstr = "{:<40} {:>8} {:>8} {:>8} {:>8} {:>8} {:>5}%  {:<16} {:<8} {:<8}\n"
    logfile.write(fmtstr.format("Filename", "NameLen", "Children", "In", "Out", "Change", "", "ACL (R,W)", "Attr", "Compress"))
    logfile.write(fmtstr.format("--------", "-------", "--------", "--", "---", "------", "", "---------", "----", "--------"))
else:
    logfile = None

# Create a file object, add it to the parent
def addFile(parent, name, sourcePath):
#    print("'{}' -> '{}'".format(targetPath, sourcePath))
    
    fileObj = FWFS.File(parent, name)
    cfg.applyRules(fileObj)
    fileObj.mtime = os.path.getmtime(sourcePath)

    with open(sourcePath, "rb") as fin:
        din = fin.read()
        ext = os.path.splitext(name)[1]
        if ext == '.json':
            dout = json.dumps(json.loads(din), separators=(',', ':')).encode()
        elif ext in ['.js']:
            dout = util.js_minify(din)
        else:
            dout = din
        fin.close()

    # If rules say we should compress this file, give it a go
    cmp = fileObj.findObject(FwObt.Compression)
    if not cmp is None:
        if cmp.compressionType() == CompressionType.gzip:
#             print("compressing '" + f.path + '"')
            dcmp = util.compress(dout)
            if len(dcmp) < len(dout):
                dout = dcmp
            else:
                # File is bigger, leave uncompressed
                fileObj.removeObject(cmp)
        else:
            print("Unsupported compression type: " + cmp.toString())
            sys.exit(1)

    fileObj.appendData(dout, len(din))

    # If required, write copy of generated file
    if outFilePath is not None:
        path = outFilePath + fileObj.path()
        if args.verbose and logfile:
            logfile.write("Writing '" + path + "'\n")
        util.mkdir(os.path.dirname(path))
        with open(path, "wb") as tmp:
            tmp.write(dout)
            tmp.close()

    return fileObj


# Create a filing system object, add it to the parent
def createFsObject(parent, target, source):
    if os.path.isdir(source):
        obj = addDirectory(parent, target, source)
    else:
        obj = addFile(parent, target, source)

    il = obj.originalDataSize()
    ol = obj.dataSize()
    if il == 0:
        pc = 0
    else:
        pc = round(100 * ol / il)

    if logfile:
        # Put long filenames on their own line
        objpath = obj.path()
        if len(objpath) > 40:
            logfile.write(objpath)
            logfile.write('\n')
            objpath = ''

        logfile.write(fmtstr.format(objpath, len(obj.name), obj.childCount(), il, ol, ol - il, pc,
                            obj.readACE().toString() + ', ' + obj.writeACE().toString(),
                            obj.attr().toString(), obj.compression().toString()))


def addDirectory(parent, name, sourcePath):
    if name == '/':
        dirObj = img.root()
    else:
        dirObj = FWFS.Directory(parent, name)
        cfg.applyRules(dirObj)

#        print("parsedir('{}', '{}')".format(target, source))
    for item in os.listdir(sourcePath):
        createFsObject(dirObj, item, os.path.join(sourcePath, item))

    return dirObj


#
cfg.applyRules(img.root())

# resolve file mappings
for target, source in cfg.sourceMap():
    createFsObject(img.root(), target, os.path.expandvars(source))

# create mount point objects
for target, store in cfg.mountPoints():
    img.root().appendMountPoint(target, store)

# Emit the image
imgFilePath = util.ospath(args.output)
print("Writing image to '" + imgFilePath + "'")
img.writeToFile(imgFilePath)

totalDataSize = img.root().totalDataSize()
totalOriginalDataSize = img.root().totalOriginalDataSize()
if totalOriginalDataSize == 0:
    pc = 0
else:
    pc = round(100 * totalDataSize / totalOriginalDataSize)
fileCount = img.root().fileCount(True)
if logfile:
    logfile.write(fmtstr.format("--------", "", "", "--", "---", "------", "", "", "", ""))
    logfile.write(fmtstr.format(str(fileCount) + " files", "", "", totalOriginalDataSize, totalDataSize, totalOriginalDataSize - totalDataSize, pc, "", "", "", ""))

print("Image contains {} objects, {} bytes in {} files ({}% of source data size)".format(img.objectCount(), totalDataSize, fileCount, pc))