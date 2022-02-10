#!/usr/bin/env python3
#
# Script to build Firmware Filesystem image
#
# See readme.md for further information
#

import os, json, sys
import util, FWFS, config
from FWFS import FwObt, isNumberType
from compress import CompressionType
from rjsmin import jsmin
import argparse

do_minify = True

def minify(name, data):
    if do_minify:
        ext = os.path.splitext(name)[1]
        if ext in ['.json', '.jsonc']:
            return json.dumps(json.loads(jsmin(data).decode()), separators=(',', ':')).encode()
        if ext in ['.js']:
            return jsmin(data)
    return data

# Create a file object, add it to the parent
def addFile(parent, name, sourcePath):
#    print("'{}' -> '{}'".format(name, sourcePath))

    fileObj = parent.findChild(name)
    if fileObj is not None:
        raise KeyError("Directory '%s' already contains file '%s'" % (parent.path(), name))

    fileObj = FWFS.File(parent, name)
    cfg.applyRules(fileObj)
    fileObj.mtime = os.path.getmtime(sourcePath)

    with open(sourcePath, "rb") as fin:
        din = fin.read()
        fin.close()
    dout = minify(name, din)

    # If rules say we should compress this file, give it a go
    cmp = fileObj.findObject(FwObt.Compression)
    if not cmp is None:
        if cmp.compressionType() == CompressionType.gzip:
#             print("compressing '" + fileObj.path() + '"')
            dcmp = util.compress(dout)
            if len(dcmp) < len(dout):
                cmp.setOriginalSize(len(dout))
                dout = dcmp
            else:
                # File is bigger, leave uncompressed
                fileObj.removeObject(cmp)
        else:
            print("Unsupported compression type: " + cmp.toString())
            sys.exit(1)

    fileObj.appendData(dout)

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
# @param parent
# @param target
# @param source Source for this file or directory, or number for mount point
def createFsObject(parent, target, source):
    # Resolve target path to single name
    while '/' in target and len(target) > 1:
        name, target = target.split('/', 1)
        dirObj = parent.findChild(name)
        if dirObj is None:
            dirObj = FWFS.Directory(parent, name)
        parent = dirObj

    if parent.findChild(target) is not None:
        raise KeyError("Error: Directory '%s' already contains '%s'" % (parent.name, target))

    if isNumberType(source):
        obj = FWFS.MountPoint(parent, target, source)
        cfg.applyRules(obj)
    elif os.path.isdir(source):
        obj = addDirectory(parent, target, source)
    else:
        obj = addFile(parent, target, source)

    if logfile:
        il = obj.originalDataSize()
        ol = obj.dataSize()
        if il == 0:
            pc = 0
        else:
            pc = round(100 * ol / il)

        # Put long filenames on their own line
        objpath = obj.path()
        if obj.obt() == FwObt.Directory:
            objpath += '/'
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

    # print("parsedir('{}', '{}')".format(target, source))
    for item in os.listdir(sourcePath):
        createFsObject(dirObj, item, os.path.join(sourcePath, item))

    return dirObj


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='Firmware Filesystem Builder')
    parser.add_argument('-l', '--log', metavar='filename', help='Create build log file, use `-` to print to screen')
    parser.add_argument('-f', '--files', metavar='directory', help='Create file layout for inspection')
    parser.add_argument('-i', '--input', metavar='filename', required=True, help='Source configuration file')
    parser.add_argument('-o', '--output', metavar='filename', required=True, help='Destination image file')
    parser.add_argument('-v', '--verbose', action='store_true', help='Show build details')
    parser.add_argument('-n', '--nominify', action='store_true', help='Do not minify Javasript or JSON')

    args = parser.parse_args()

    if args.verbose:
        print("Python version: ", sys.version, ", version info: ", sys.version_info)

    do_minify = not args.nominify

    # Parse the configuration file or input JSON
    cfg = config.Config(args.input)

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

    #
    cfg.applyRules(img.root())

    # resolve file mappings
    for target, source in cfg.sourceMap():
        if logfile:
            logfile.write(">> '{}' -> '{}'\n".format(target, source))
        createFsObject(img.root(), target, os.path.expandvars(source))

    # create mount point objects
    for target, store in cfg.mountPoints():
        createFsObject(img.root(), target, store)

    # Emit the image
    imgFilePath = util.ospath(args.output)
    if args.verbose:
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
        logfile.write(fmtstr.format(str(fileCount) + " files", "", "", totalOriginalDataSize, totalDataSize, totalOriginalDataSize - totalDataSize, pc, "", "", ""))

    print("Image '%s' contains %u objects, %u bytes in %u files (%u%% of source data size)" % (imgFilePath, img.objectCount(), totalDataSize, fileCount, pc))

