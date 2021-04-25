HWCONFIG := fstest

# Empty SPIFFS partition please
SPIFF_FILES :=

# Add 16 bytes user attribute space
SPIFFS_OBJ_META_LEN := 32

COMPONENT_INCDIRS := include
COMPONENT_SRCDIRS := app modules

ifeq ($(SMING_ARCH),Host)
COMPONENT_SRCDIRS += modules/Host
endif

# Don't need network
HOST_NETWORK_OPTIONS := --nonet

COMPONENT_DEPENDS := \
	SmingTest \
	Spiffs \
	LittleFS

# Time in milliseconds to pause after a test group has completed
CONFIG_VARS += TEST_GROUP_INTERVAL
TEST_GROUP_INTERVAL ?= 500
APP_CFLAGS += -DTEST_GROUP_INTERVAL=$(TEST_GROUP_INTERVAL)

# Time in milliseconds to wait before re-starting all tests
# Set to 0 to perform a system restart after all tests have completed
CONFIG_VARS += RESTART_DELAY
ifndef RESTART_DELAY
ifeq ($(SMING_ARCH),Host)
RESTART_DELAY = 0
else
RESTART_DELAY ?= 10000
endif
endif
APP_CFLAGS += -DRESTART_DELAY=$(RESTART_DELAY)

.PHONY: execute
execute: flash run

# This gets referred to using IMPORT_FSTR so need to build before code is compiled
all: out/fwfsImage1.bin

out/fwfsImage1.bin: out/backup.fwfs.bin out/large-random.bin
out/backup.fwfs.bin:
	$(Q) $(FSBUILD) -i backup.fwfs -o $@
# Checks Data24 so size needs to be >= 0x10000
out/large-random.bin:
	openssl rand -out $@ $$((0x12340))

clean: fstest-clean
.PHONY: fstest-clean
fstest-clean:
	$(Q) rm -f out/*.bin
