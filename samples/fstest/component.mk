ifneq ($(SMING_ARCH),Host)
$(error Only Host mode supported)
endif

DISABLE_SPIFFS := 1
COMPONENT_DEPENDS := IFS
HOST_NETWORK_OPTIONS := --nonet

FWFILES := out/fwfsImage1.bin

App-build: $(FWFILES)

.PHONY: fsbuild
fsbuild: $(FWFILES) ##Build firmware filesystem image (use -B to rebuild)

$(FWFILES):
	$(FSBUILD) -i $(basename $(@F)).ini -o $@ $(if $V,--verbose -l -)
	touch app/application.cpp

