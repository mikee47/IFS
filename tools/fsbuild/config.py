#
# Filesystem builder configuration file support
#
# Defines all options for the build
#

import sys, os, json
from fnmatch import fnmatch
from FWFS import ObjectAttr
from rjsmin import jsmin

class Config:
    def __init__(self, filename):
        din = open(filename).read()
        self.data = json.loads(jsmin(din))

        try:
            from jsonschema import Draft7Validator
            # Validate configuration against schema
            schemaPath = os.path.dirname(__file__) + '/schema.json'
            schema = json.load(open(schemaPath))
            v = Draft7Validator(schema)
            errors = sorted(v.iter_errors(self.data), key=lambda e: e.path)
            if errors != []:
                for e in errors:
                    sys.stderr.write("%s @ %s\n" % (e.message, e.path))
                sys.exit(1)
        except ImportError as err:
            sys.stderr.write("\n** WARNING! %s: Cannot validate config '%s', please run `make python-requirements` **\n\n" % (str(err), filename))

    def sourceMap(self):
        """ Each entry consists of "{target}": "{source}", where
            {target} is the name to be used on the filesystem image,
            and {source} is a file or folder to obtain the content."""
        return self.data['source'].items()

    def mountPoints(self):
        """Mount points create a virtual folder which is redirected to another object store"""
        return self.data.get('mountpoints', {}).items()

    def volumeName(self):
        return self.data['name']

    def volumeID(self):
        id = self.data.get('id', 0)
        return eval(id) if type(id) is str else id

    # Apply rules to given IFS.File object
    def applyRules(self, f):
        def match(mask):
            if type(mask) is list:
                for m in mask:
                    if match(m):
                        return True
                return False

            if fnmatch(f.path(), mask):
                # print("'%s' matches '%s'" % (f.path(), mask))
                return True

            # print("'%s', '%s'" % (f.name, mask))
            if mask[0:1] != '/' and fnmatch(f.name, mask):
                # print("'%s' matches '%s'" % (f.name, mask))
                return True

            if mask == '/' and f.path() == '':
                return True

            return False


        for rule in self.data.get('rules', []):
            if match(rule['mask']):
                value = rule.get('readonly')
                if value is not None:
                    # print("Set readonly = %d" % value)
                    f.setAttr(ObjectAttr.ReadOnly, value)
                value = rule.get('compress')
                if value is not None:
                    # print("Set compression = %s" % value)
                    f.appendCompression(value)
                value = rule.get('read')
                if value is not None:
                    # print("Set read ACE = %s" % value)
                    f.appendReadACE(value)
                value = rule.get('write')
                if value is not None:
                    # print("Set write ACE = %s" % value)
                    f.appendWriteACE(value)
