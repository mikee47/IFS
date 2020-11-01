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

DEBUG_VARS += FSBUILD_PATH
FSBUILD_PATH := $(COMPONENT_PATH)/tools/fsbuild/fsbuild.py
FSBUILD := python3 $(FSBUILD_PATH)
