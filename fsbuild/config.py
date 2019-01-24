#
# Filesystem builder configuration file support
#
# Defines all options for the build
#

import ConfigParser, sys, util, os
from fnmatch import fnmatch
from FWFS import ObjectAttr


# An individual field specification
class FieldSpec:
    __field = ''
    __value = ''

    def __init__(self, spec):
        pair = spec.split('=')
        self.__field = pair[0].strip()
        self.__value = pair[1].strip().lower()
#        print "    " + self.__field + " = " + self.__value

    def apply(self, named):
#        print "apply to '{}': {} = {}".format(named.path, self.__field, self.__value)
        if self.__field == 'readonly':
            named.setAttr(ObjectAttr.ReadOnly, self.__value)
        elif self.__field == 'compress':
            named.appendCompression(self.__value)
        elif self.__field == 'read':
            named.appendReadACE(self.__value)
        elif self.__field == 'write':
            named.appendWriteACE(self.__value)
        else:
            print "Unknown field '{}' in rule".format(self.__field)
            sys.exit(1)
        
    def toString(self):
        return self.__field + " = " + self.__value


# Encapsulates a single rule of the form:
#    {path masks}: {field specs}
#
# Where {path masks} is a comma-separated list of unix-style filename wildcards
#       {field specs} is a comma-separated list, each specifiying the field to modify
#            For ACL entries:
#               r={user role}
#               w={user role}
#            For attributes:
#                attr={attribute string}
#                {attribute string] can contain the attribute characters in any order,
#                optionally prefixed by '-' to remove the attribute.
#
class Rule:
#    __masks = []  # File path masks this rule applies to
#    __specs = []  # The field specs

    def __init__(self, strPathMasks, strFieldSpecs):
#        print "Rule: {} : {}".format(strPathMasks, strFieldSpecs)

        self.__masks = []
        masks = strPathMasks.split(',')
        for mask in masks:
            self.__masks.append(mask.strip())

        self.__specs = []
        specs = strFieldSpecs.split(',')
        for spec in specs:
            self.__specs.append(FieldSpec(spec))

    def match(self, named):
        for mask in self.__masks:
            if fnmatch(named.path(), mask):
#                print "'{}' matches '{}'".format(named.path(), mask)
                return True
        return False

    def apply(self, f):
        for spec in self.__specs:
            spec.apply(f)
            
    def toString(self):
        s = "".join(self.__masks) + ": ";
        for spec in self.__specs:
            s += spec.toString() + ", "
        return s


class Config:
#    __file = ConfigParser.RawConfigParser()
#    __rules = []

    def __init__(self, filename):
        self.__file = ConfigParser.RawConfigParser()
        self.__file.readfp(open(filename))
        
        # Pull rule specifications into objects for efficient parsing
        rules = self.__file.items('rules')
        self.__rules = []
        for item in rules:
            rule = Rule(item[0], item[1])
            self.__rules.append(rule)
            
#         for rule in self.__rules:
#             print rule.toString()
            

    def sourceMap(self):
        """ File mappings are specified in a separate section.
            Each entry consists of {target}: {source}, where
            {target} is the name to be used on the filesystem image,
            and {source} is a file or folder to obtain the content."""
        return self.__file.items('source')

    def mountPoints(self):
        """Mount points create a virtual folder which is redirected to another object store"""
        return self.__file.items("mountpoints")

    # Generated image file
    def imageFilePath(self):
        return util.ospath(self.__readConfig('image', 'out/fwfiles.bin'))

    # folder to write generated files (optional)
    def destPath(self):
        tmp = util.ospath(self.__readConfig('dest', ''))
        return os.path.expandvars(tmp)
    
    def volumeName(self):
        return self.__readConfig('volumeName', 'FWFS')

    def volumeID(self):
        s = self.__readConfig('volumeID', '0')
        return int(s, 16)

    # Apply rules to given IFS.File object
    def applyRules(self, f):
        for rule in self.__rules:
            if rule.match(f):
                rule.apply(f)

    def __readConfig(self, option, default):
        if self.__file.has_option('config', option):
            return self.__file.get('config', option)
        else:
            return default


