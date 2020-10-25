DISABLE_SPIFFS := 1
COMPONENT_DEPENDS := IFS

ifneq ($(SMING_ARCH),Host)
$(error Only Host mode supported)
endif
