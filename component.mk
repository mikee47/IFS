COMPONENT_DEPENDS := \
	FlashString \
	spiffs

COMPONENT_SRCDIRS := \
	src \
	src/File \
	src/FWFS \
	src/HYFS \
	src/SPIFFS \
	src/Arch/$(SMING_ARCH)

COMPONENT_INCDIRS := \
	src/include \
	src/Arch/$(SMING_ARCH)/include

# Defined in spiffs Component
COMPONENT_RELINK_VARS += SPIFFS_OBJ_META_LEN
COMPONENT_CXXFLAGS += -DSPIFFS_OBJ_META_LEN=$(SPIFFS_OBJ_META_LEN)

COMPONENT_DOCFILES := tools/fsbuild/README.rst
COMPONENT_DOXYGEN_INPUT := src

##@Building

DEBUG_VARS += FSBUILD
FSBUILD_PATH := $(COMPONENT_PATH)/tools/fsbuild/fsbuild.py
FSBUILD := $(PYTHON) $(FSBUILD_PATH)

# Target invoked via partition table
ifneq (,$(filter fwfs-build,$(MAKECMDGOALS)))
PART_TARGET := $(PARTITION_$(PART)_FILENAME)
ifneq (,$(PART_TARGET))
$(eval PART_CONFIG := $(call HwExpr,part.build['config']))
# PART_CONFIG := $(call AbsoluteSourcePath,$(PROJECT_DIR),$(PART_CONFIG))
.PHONY: fwfs-build
fwfs-build:
	@echo "Creating FWFS image '$(PART_TARGET)'"
	$(Q) $(FSBUILD) -i $(PART_CONFIG) -o $(PART_TARGET) $(if $V,--verbose -l -)
endif
endif
