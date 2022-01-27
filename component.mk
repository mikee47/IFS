COMPONENT_SRCDIRS := \
	src \
	src/File \
	src/FWFS \
	src/HYFS \
	src/Arch/$(SMING_ARCH)

COMPONENT_INCDIRS := \
	src/include \
	src/Arch/$(SMING_ARCH)/include

ifeq ($(SMING_ARCH),Host)
ifeq ($(UNAME),Windows)
	EXTRA_LIBS	+= ntdll
	COMPONENT_SRCDIRS += src/Arch/Host/Windows
endif
endif

COMPONENT_DOCFILES := tools/fsbuild/README.rst
COMPONENT_DOXYGEN_INPUT := src

# Extended debug information level
COMPONENT_RELINK_VARS += FWFS_DEBUG
FWFS_DEBUG ?= 0
COMPONENT_CXXFLAGS += -DFWFS_DEBUG=$(FWFS_DEBUG)

##@Building

HWCONFIG_BUILDSPECS += $(COMPONENT_PATH)/build.json

DEBUG_VARS += FSBUILD
FSBUILD_PATH := $(COMPONENT_PATH)/tools/fsbuild/fsbuild.py
FSBUILD := $(PYTHON) $(FSBUILD_PATH) $(if $V,--verbose -l -)

CACHE_VARS += FSBUILD_OPTIONS
FSBUILD_OPTIONS ?=

# Target invoked via partition table
ifneq (,$(filter fwfs-build,$(MAKECMDGOALS)))
PART_TARGET := $(PARTITION_$(PART)_FILENAME)
ifneq (,$(PART_TARGET))
$(eval PART_CONFIG := $(call HwExpr,part.build['config']))
.PHONY: fwfs-build
fwfs-build:
	@echo "Creating FWFS image '$(PART_TARGET)'"
	$(Q) $(FSBUILD) $(FSBUILD_OPTIONS) -i "$(subst ",\",$(PART_CONFIG))" -o $(PART_TARGET)
endif
endif
