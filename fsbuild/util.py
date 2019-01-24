#
# General filesystem utilities
#

import sys, os, io, gzip
sys.path.append(os.path.join(os.path.dirname(__file__), 'rjsmin'))
import rjsmin

def alignUp(n):
    return (n + 3) & ~0x00000003

def pad(x):
    return x.ljust(alignUp(len(x)), '\0')

def _BV(x):
    return 1 << x

def ospath(path):
    """Ensure a path contains valid OS path separators"""
    return path.replace('/', os.path.sep)

def mkdir(path):
    """Create a directory with any subdirectories as well"""
    if not os.path.isdir(path):
        os.makedirs(path)

def cleandir(root):
    """Remove contents of a directory, including any sub-directories
        Does not remove the root"""
    for name in os.listdir(root):
        path = os.path.join(root, name)
        if os.path.isdir(path):
            cleandir(path)
            os.rmdir(path)
        else:
            os.remove(path)


# Update file timestamp so it gets re-read by make
def touch(fname, times = None):
    with open(fname, 'a'):
        os.utime(fname, times)


# Compress data using GZip
def compress(data):
    tmp = io.BytesIO()
    with gzip.GzipFile(fileobj = tmp, mode = 'wb') as gz:
        gz.write(data)
        gz.close()
    tmp.seek(0)
    return tmp.read()


# Minify a javascript file
def js_minify(src):
    return rjsmin.jsmin(src)



# drill into each directory recursively so that each lower level file is emitted first, returning its index
# this list is added to the directory object, which is emitted last

def parseMap(target, sourceRoot, callback):
    """ resolve a file mapping which can be a file or folder
        callback is invoked for each resolved file
        returns size of data read"""
    def parsedir(target, source):
        if target == '/':
            target = ""
        size = 0
#        print "parsedir('{}', '{}')".format(target, source)
        sourcePath = os.path.join(sourceRoot, source)
        for name in os.listdir(sourcePath):
            sourcePath = os.path.join(sourceRoot, source, name)
            if target == "":
                targetPath = name
            else:
                targetPath = target + '/' + name
            if os.path.isdir(sourcePath):
                size += parsedir(targetPath, os.path.join(source, name))
            else:
                size += callback(targetPath, sourcePath)
    
        return size
    
    aa

    sourceRoot = os.path.expandvars(sourceRoot)
    if os.path.isdir(sourceRoot):
        return parsedir(target, "")
    else:
        return callback(target, sourceRoot)
    

