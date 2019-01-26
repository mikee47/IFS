#
# Makefile for fscopy
#

CC := gcc
CXX := g++
LD := gcc

CFLAGS :=	-O2 -Wall -Wno-unused-value \
			-DSDK_INTERNAL \
			-D__ets__ \
			-DARDUINO=106

CXXFLAGS := $(CFLAGS)

ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

ifeq ($(OS),Windows_NT)
  # Convert Windows paths to POSIX paths
  SMING_HOME := $(subst \,/,$(addprefix /,$(subst :,,$(SMING_HOME))))
  SMING_HOME := $(subst //,/,$(SMING_HOME))
endif

SDK_BASE = $(SMING_HOME)/third-party/ESP8266_NONOS_SDK

BUILD_BASE := out
MODULES :=		$(SMING_HOME)/Services/IFS \
				$(SMING_HOME)/Services/SpifFS \
				$(SMING_HOME)/third-party/spiffs/src \
				$(SMING_HOME)/Wiring \
				$(SMING_HOME)/SmingCore

SRC_DIR := 		$(MODULES)
BUILD_DIR :=	$(subst $(SMING_HOME),,$(addprefix $(BUILD_BASE),$(MODULES)))

SMING_INCDIR = 	include \
				system/include \
				SmingCore \
				third-party/esp-open-lwip/include \
				third-party/rboot \
				third-party/rboot/appcode

SDK_INCDIR = 	include

INCDIR	:= $(addprefix -I,$(SRC_DIR)) \
			$(SMING_HOME) \
			$(addprefix -I$(SMING_HOME)/,$(SMING_INCDIR)) \
			$(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))


C_SRC	:= $(subst $(SMING_HOME)/,,$(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c)))
CXX_SRC	:= $(subst $(SMING_HOME)/,,$(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))) 
C_OBJ	:= $(patsubst %.c,$(BUILD_BASE)/%.o,$(C_SRC))
CXX_OBJ	:= $(patsubst %.cpp,$(BUILD_BASE)/%.o,$(CXX_SRC))
OBJ		:= $(C_OBJ) $(CXX_OBJ)


vpath %.c $(SRC_DIR)
vpath %.cpp $(SRC_DIR)

define compile-objects
$1/%.o: %.c $1/%.c.d
	$(vecho) "CC $$<"
	$(Q) $(CC) $(INCDIR) $(CFLAGS) -c $$< -o $$@
$1/%.o: %.cpp $1/%.cpp.d
	$(vecho) "C+ $$<"
	$(Q) $(CXX) $(INCDIR) $(CXXFLAGS) -c $$< -o $$@
$1/%.c.d: %.c
	$(Q) $(CC) $(INCDIR) $(CFLAGS) -MM -MT $1/$$*.o $$< -o $$@
$1/%.cpp.d: %.cpp
	$(Q) $(CXX) $(INCDIR) $(CXXFLAGS) -MM -MT $1/$$*.o $$< -o $$@
 
.PRECIOUS: $1/%.c.d $1/%.cpp.d
endef

PHONY:

check:
	echo $(BUILD_DIR)
	echo $(OBJ)
	echo $(INCDIR)

all: checkdirs fscopy

fscopy: $(OBJ) 
	$(vecho) "LD $@"
	$(Q) $(LD) -o $@ $^
	
checkdirs: $(BUILD_DIR)

$(BUILD_DIR):
	$(Q) mkdir -p $@


clean:
	$(Q) rm -r -f $(BUILD_BASE)/*


$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))
$(foreach bdir,$(BUILD_DIR),$(eval include $(wildcard $(bdir)/*.c.d)))
$(foreach bdir,$(BUILD_DIR),$(eval include $(wildcard $(bdir)/*.cpp.d)))
