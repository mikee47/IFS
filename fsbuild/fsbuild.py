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

if len(sys.argv) > 1:
    configFile = sys.argv[1]
else:
    configFile = 'fsbuild.ini'

cfg = config.Config(configFile)

img = FWFS.Image(cfg.volumeName(), cfg.volumeID())

dstpath = cfg.destPath()
if dstpath != '':
    print "Writing copy of generated files to '" + dstpath + '"'
    util.mkdir(dstpath)
    util.cleandir(dstpath)


fmtstr = "{:<40} {:>8} {:>8} {:>8} {:>8} {:>8} {:>5}%  {:<16} {:<8} {:<8}"
print fmtstr.format("Filename", "NameLen", "Children", "In", "Out", "Change", "", "ACL (R,W)", "Attr", "Compress")
print fmtstr.format("--------", "-------", "--------", "--", "---", "------", "", "---------", "----", "--------")

# Create a file object, add it to the parent
def addFile(parent, name, sourcePath):
#    print "'{}' -> '{}'".format(targetPath, sourcePath)
    
    fileObj = FWFS.File(parent, name)
    cfg.applyRules(fileObj)
    fileObj.mtime = os.path.getmtime(sourcePath)

    with open(sourcePath, "rb") as fin:
        din = fin.read()
        ext = os.path.splitext(name)[1]
        if ext == '.json':
            dout = json.dumps(json.loads(din), separators = (',', ':'))
        elif ext in ['.js']:
            dout = util.js_minify(din)
        else:
            dout = din
        fin.close()

    # If rules say we should compress this file, give it a go
    cmp = fileObj.findObject(FwObt.Compression)
    if not cmp is None:
        if cmp.compressionType() == CompressionType.gzip:
#             print "compressing '" + f.path + '"'
            dcmp = util.compress(dout)
            if len(dcmp) < len(dout):
                dout = dcmp
            else:
                # File is bigger, leave uncompressed
                fileObj.removeObject(cmp)
        else:
            print "Unsupported compression type: " + cmp.toString()
            sys.exit(1)

    fileObj.appendData(dout, len(din))

    # If required, write copy of generated file
#    global dstpath
    if dstpath != '':
        path = dstpath + util.ospath(fileObj.path())
        print "Writing '" + path + "'"
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
        pc = 100 * (ol - il) / il

    # Put long filenames on their own line
    objpath = obj.path()
    if len(objpath) > 40:
        print objpath
        objpath = ''

    print fmtstr.format(objpath, len(obj.name), obj.childCount(), il, ol, ol - il, pc, 
                        obj.readACE().toString() + ', ' + obj.writeACE().toString(),
                        obj.attr().toString(), obj.compression().toString())


def addDirectory(parent, name, sourcePath):
    if name == '/':
        dirObj = img.root()
    else:
        dirObj = FWFS.Directory(parent, name)
        cfg.applyRules(dirObj)

#        print "parsedir('{}', '{}')".format(target, source)
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
imgFilePath = cfg.imageFilePath()
print "Writing image to '" + imgFilePath + "'"
img.writeToFile(imgFilePath)


totalDataSize = img.root().totalDataSize()
totalOriginalDataSize = img.root().totalOriginalDataSize()
if totalOriginalDataSize == 0:
    pc = 0
else:
    pc = 100 * (totalDataSize - totalOriginalDataSize) / totalOriginalDataSize
print fmtstr.format("--------", "", "", "--", "---", "------", "", "", "", "")
print fmtstr.format(str(img.root().fileCount(True)) + " files", "", "", totalOriginalDataSize, totalDataSize, totalOriginalDataSize - totalDataSize, pc, "", "", "", "")

print "Image contains {} objects".format(img.objectCount())
