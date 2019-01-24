
#
# Definitions for access control
#

from enum import enum

#
UserRole = enum(
    none = 0,
    any = 0,
    guest = 1,
    user = 2,
    manager = 3,
    admin = 4
)

