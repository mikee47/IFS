#
# Enums aren't supported in python 2.7 so we'll use our own approach
#

def enum(**enums):
    enums['strings'] = dict((value, key) for key, value in enums.iteritems())
    return type('Enum', (), enums)
