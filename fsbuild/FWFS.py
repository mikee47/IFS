#
# Firmware Filesystem support
#

import struct, sys, util, time, numbers
from enum import IntEnum
from util import _BV
from access import UserRole
from compress import CompressionType

def isNumberType(obj):
    return isinstance(obj, numbers.Number)

# Images start with a single word to identify this as a Firmware Filesystem image
SYS_START_MARKER = 0x53465746  # "FWFS"

# Image end marker (reverse of magic)
SYS_END_MARKER = 0x46574653  # "SFWF"

# Object type has top bit set for object reference, if clear it's stored directly
FWOBT_REF = 0x80

# Object types
class FwObt(IntEnum):
    # 1-byte sized
    End = 0,  # The system image footer
    Data8 = 1,
    ID32 = 2, # e.g.volume ID
    ObjAttr = 3,
    Compression = 4,
    ReadACE = 5,  # minimum UserRole for read access
    WriteACE = 6,  # minimum UserRole for write access
    ObjectStore = 7, # Identifier for object store
    # 2-byte sized
    Data16 = 32,
    # Named
    Volume = 33,  # Volume, top-level container
    MountPoint = 34, #
    Directory = 35,  # Directory entry
    File = 36,  # File entry
    # 3-byte sized
    Data24 = 64

FwObt_Named = FwObt.Volume

class ObjectAttr(IntEnum):
    ReadOnly = 0,
    Archive = 1

ObjectAttrChars = {
    ObjectAttr.ReadOnly: 'R',
    ObjectAttr.Archive: 'A'
}


# Convert file attributes into printable string
def objectAttrToString(attr):
    s = ''
    for a in ObjectAttrChars:
        if attr & _BV(a):
            s += ObjectAttrChars[a]
        else:
            s += '-'
    return s


def objectAttrFromChar(c):
    for a in ObjectAttrChars:
        if c.upper() == ObjectAttrChars[a]:
            return a
    print("Unknown object attribute '{}'".format(c))
    sys.exit(1)



class Object(object):
    def __init__(self, parent, obt, content = ''):
        self.__parent = parent
        self.__obt = obt
        self.__offset = 0
        if isinstance(content, bytes):
            self.__content = content
        else:
            self.__content = content.encode('utf-8')
        self.isRef = True
        if parent is not None:
            parent.appendObject(self)
            
    def parent(self):
        return self.__parent

    def obt(self):
        return self.__obt
    
    def isNamed(self):
        return self.__obt >= FwObt_Named

    def offset(self):
        return self.__offset

    def id(self):
        return self.__id

    def header(self):
        """Get the size bytes (8, 16 or 32) - everything else is content"""
        return struct.pack("<B", self.__obt)

    def refHeader(self):
        assert(self.__id is not None)
        return struct.pack("<BBH", self.__obt | FWOBT_REF, 2, self.__id)

    def contentSize(self):
        return len(self.content())

    def content(self):
        return self.__content

    def emit(self, image):
        # Check we haven't already been emitted
        if self.__offset != 0:
            return
#        print("emit " + FwObt.strings[self.obt()])
        contentSize = self.contentSize()
        if contentSize > self.maxContentSize():
            print("Content too big: object is {} bytes, maximum is {}".format(contentSize, self.maxContentSize()))
        self.__offset = image.offset()
        self.__id = image.writeObject(self.header(), self.content())


class Object8(Object):
    def __init__(self, parent, obt, content = ""):
        super(Object8, self).__init__(parent, obt, content)
        self.isRef = False

    def header(self):
        return Object.header(self) + struct.pack("<B", self.contentSize())
    
    def maxContentSize(self):
        return 0xff

    def size(self):
        return 2 + self.contentSize()


class Object16(Object):
    def header(self):
        return Object.header(self) + struct.pack("<H", self.contentSize())

    def maxContentSize(self):
        return 0xffff

    def size(self):
        return 3 + self.contentSize()


class Object24(Object):
    def header(self):
        contentSize = self.contentSize()
        return Object.header(self) + struct.pack("<HB", contentSize & 0xffff, contentSize >> 16)

    def maxContentSize(self):
        return 0xffffff

    def size(self):
        return 4 + self.contentSize()


class AttrObject(Object8):
    def __init__(self, parent):
        super(AttrObject, self).__init__(parent, FwObt.ObjAttr)
        self.__attr = 0

    # Update attributes from a string
    def update(self, strAttr):
#        self.__attr = updateObjectAttr(self.__attr, strAttr)
        bset = True
        for c in strAttr:
            if c == '-':
                # Applies only to the next attribute character
                bset = False
            else:
                self.set(objectAttrFromChar(c), bset)
    #                print("setAttr({}, {})".format(attr, bset))
                bset = True

        
    def attr(self):
        return self.__attr
        
    def content(self):
        return struct.pack("<B", self.__attr)

    def toString(self):
        return objectAttrToString(self.__attr)

    def get(self, oa):
        return (self.__attr & _BV(oa)) != 0
    
    def set(self, oa, state = True):
        if state:
            self.__attr |= _BV(oa)
        else:
            self.__attr &= ~_BV(oa)
    
    def clear(self, oa):
        self.set(oa, False)
    
    

class CompressionObject(Object8):
    def __init__(self, parent, s = CompressionType.none):
        super(CompressionObject, self).__init__(parent, FwObt.Compression)
        if isNumberType(s):
            self.__compressionType = s
        else:
            self.__compressionType = CompressionType.__members__[s]

    def content(self):
        return struct.pack("<B", self.__compressionType)

    def compressionType(self):
        return self.__compressionType

    def toString(self):
        return self.__compressionType.name


class AceObject(Object8):
    def __init__(self, parent, obt, s):
        super(AceObject, self).__init__(parent, obt)
        if isNumberType(s):
            self.__role = s
        else:
            self.__role = UserRole.__members__[s]
#        print "Adding ACE {}, {}, {}".format(obt, s, self.toString())

    def content(self):
        return struct.pack("<B", self.__role)

    def role(self):
        return self.__role

    def toString(self):
        return self.__role.name


class ObjectStoreObject(Object8):
    """Identifies an object store for a mount point"""
    def __init__(self, parent, store):
        super(ObjectStoreObject, self).__init__(parent, FwObt.ObjectStore)
        if isNumberType(store):
            self.__store = store
        else:
            self.__store = int(store)

    def content(self):
        return struct.pack("<B", self.__store)

    def store(self):
        return self.__store


class ID32Object(Object8):
    def __init__(self, parent, obt, value):
        super(ID32Object, self).__init__(parent, obt)
        self.__value = value

    def content(self):
        return struct.pack("<L", self.__value)

    def value(self):
        return self.__value


class EndObject(Object8):
    def __init__(self, parent, checksum):
        super(EndObject, self).__init__(parent, FwObt.End)
        self.__checksum = checksum

    def content(self):
        return struct.pack("<L", self.__checksum)

    def checksum(self):
        return self.__checsum



class NamedObject(Object16):
    def __init__(self, parent, obt, name):
        super(NamedObject, self).__init__(parent, obt)
        self.__children = []
        self.name = name
        self.mtime = time.time()
        self.__originalDataSize = 0;
        self.__dataSize = 0

    def pathsep(self):
        return ':'

    def path(self):
        s = self.parent().path() + self.parent().pathsep() + self.name
        return s
   
    def setAttr(self, attr, state):
        obj = self.findObject(FwObt.ObjAttr)
        if obj is None:
            obj = AttrObject(self) 
        obj.set(attr, state)

    def attr(self):
        """Get attribute object, or temporary empty one if there isn't one"""
        obj = self.findObject(FwObt.ObjAttr)
        if obj is None:
            obj = AttrObject(None)
        return obj
        
    
    def compression(self):
        """Get compression object, or temporary empty one if there isn't one"""
        obj = self.findObject(FwObt.Compression)
        if obj is None:
            obj = CompressionObject(None)
        return obj

    
    def childTableSize(self):
        size = 0
        for obj in self.__children:
            if obj.isRef:
                size += 4
            else:
                size += util.alignUp(obj.size())
        return size

    def childTable(self):
        table = b''
        for obj in self.__children:
            if obj.isRef:
                table += obj.refHeader()
            else:
                table += util.pad(obj.header() + obj.content())
        cts = self.childTableSize()
        if len(table) != cts:
            print("len(table) = {}, cts = {}".format(len(table), cts))
            assert(False)
        return table

    def contentSize(self):
        # temporary: children are all references
        return 5 + util.alignUp(len(self.name)) + self.childTableSize()
        
    # Fetch the content - must be called after object indices have been assigned
    def content(self):
        s = struct.pack("<BL", len(self.name), round(self.mtime))
        s += util.pad(self.name.encode())
        s += self.childTable()
        return s;

    def appendObject(self, obj):
        self.__children.append(obj)
        
    def removeObject(self, obj):
        self.__children.remove(obj)

    def appendCompression(self, compressionType):
        obj = self.findObject(FwObt.Compression);
        if not obj is None:
            self.removeObject(obj)
        obj = CompressionObject(self, compressionType)
        if obj.compressionType() == CompressionType.none:
            self.removeObject(obj)

    def appendACE(self, obt, role):
        if not isNumberType(role):
            role = UserRole.__members__[role]
        ace = self.findInheritableObject(obt)
        if ace is None or ace.role() != role:
            AceObject(self, obt, role)

    def appendReadACE(self, role):
        self.appendACE(FwObt.ReadACE, role)

    def appendWriteACE(self, role):
        self.appendACE(FwObt.WriteACE, role)

    def readACE(self):
        return self.findInheritableObject(FwObt.ReadACE)

    def writeACE(self):
        return self.findInheritableObject(FwObt.WriteACE)

    def appendMountPoint(self, target, store):
        mp = MountPoint(self, target)
        ObjectStoreObject(mp, store)

    def findInheritableObject(self, obt):
        obj = self.findObject(obt)
        if obj is None and self.parent() is not None:
            obj = self.parent().findInheritableObject(obt);
        return obj

    def findObject(self, obt):
        for child in self.__children:
            if child.obt() == obt:
                return child
        return None

    def appendData(self, content, originalSize):
        length = len(content)
        if length <= 0xff:
            Object8(self, FwObt.Data8, content)
        elif length <= 0xffff:
            Object16(self, FwObt.Data16, content)
        elif length <= 0xffffff:
            Object24(self, FwObt.Data24, content)
        else:
            print("Object data too large")
            exit(1)

        self.__originalDataSize += originalSize;
        self.__dataSize += len(content)


    def childCount(self):
        return len(self.__children)

    def fileCount(self, recursive):
        count = 0
        for child in self.__children:
            if child.obt() == FwObt.File:
                count += 1
            if recursive and child.isNamed():
                count += child.fileCount(True)
        return count

    def dataSize(self):
        return self.__dataSize;

    def originalDataSize(self):
        return self.__originalDataSize;

    def totalChildCount(self):
        total = self.childCount()
        for child in self.__children:
            if child.isNamed():
                total += child.totalChildCount()
        return total

    def totalDataSize(self):
        total = self.dataSize()
        for child in self.__children:
            if child.isNamed():
                total += child.totalDataSize()
        return total

    def totalOriginalDataSize(self):
        total = self.originalDataSize()
        for child in self.__children:
            if child.isNamed():
                total += child.totalOriginalDataSize()
        return total

    def emit(self, image):
        for child in self.__children:
            if child.isRef:
                child.emit(image)
        Object.emit(self, image)

    #
    def toString(self):
        s = self.path.ljust(32)
        s += str(self.size).ljust(8)
        s += self.acl.toString().ljust(20)
        s += objectAttrToString(self.attr)
        return s


class Volume(NamedObject):
    def __init__(self, name):
        super(Volume, self).__init__(None, FwObt.Volume, name)

    def pathsep(self):
        return ''

    def path(self):
        return ""


class Directory(NamedObject):
    """Directory object"""

    def __init__(self, parent, name):
        super(Directory, self).__init__(parent, FwObt.Directory, name)

    def pathsep(self):
        return '/'


class MountPoint(NamedObject):
    """Mount point object"""
    
    def __init__(self, parent, name):
        super(MountPoint, self).__init__(parent, FwObt.MountPoint, name)

    def pathsep(self):
        return '/'


class File(NamedObject):
    """File object"""

    def __init__(self, parent, name):
        super(File, self).__init__(parent, FwObt.File, name)


# Used to build a Firmware file image
class Image:
    def __init__(self, volumeName, volumeID):
        self.__objectCount = 0  # Number of objects written
        self.__checksum = 0  # @todo update this from objects
        self.__vol = Volume(volumeName)
        ID32Object(self.__vol, FwObt.ID32, volumeID)
        self.__root = Directory(self.__vol, "")
        self.__root.appendReadACE(UserRole.guest)
        self.__root.appendWriteACE(UserRole.admin)

    def objectCount(self):
        return self.__objectCount

    def writeToFile(self, filename):
        self.__fout = open(filename, "wb")
        self.__fout.write(struct.pack("<L", SYS_START_MARKER))
        self.__vol.emit(self)
        end = EndObject(None, self.__checksum)
        end.emit(self)
        self.__fout.write(struct.pack("<L", SYS_END_MARKER))
        self.__fout.close()
        self.__fout = None

    def root(self):
        return self.__root

    def offset(self):
        return self.__fout.tell()

    # Return object ID (1-based)
    def writeObject(self, header, content):
        self.__fout.write(header)
        self.__fout.write(content)
        size = len(header) + len(content)
        self.__fout.write(b''.ljust(util.alignUp(size) - size, b'\0'))
        self.__objectCount += 1
        objID = self.__objectCount
        return objID

