#
# General filesystem utilities
#

import sys, os, io, gzip, platform

def _BV(x):
    return 1 << x

def fixpath(path):
    """Paths in Windows can get a little weird """
    if path[0] == '/' and platform.system() == 'Windows':
        return path[1] + ':' + path[2:]
    return path

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
