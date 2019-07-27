
#
# Definitions for access control
#

from enum import IntEnum

#
class UserRole(IntEnum):
    none = 0,
    any = 0,
    guest = 1,
    user = 2,
    manager = 3,
    admin = 4

