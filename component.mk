COMPONENT_DEPENDS := spiffs

COMPONENT_SRCDIRS := \
	src \
	src/Arch/$(SMING_ARCH)

COMPONENT_INCDIRS := \
	src/include \
	src/Arch/$(SMING_ARCH)/include

FSBUILD := $(COMPONENT_PATH)/tools/fsbuild/fsbuild.py