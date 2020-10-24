
Q ?= @
HOST_CC = gcc
HOST_CXX = g++
HOST_LD = g++
CFLAGS = -Wall -D_POSIX_C_SOURCE=200809L -O0 -g3 -m32
CXXFLAGS = $(CFLAGS) -std=c++11

INCDIR := ../src ../posix ../../spiffs/src
INCDIR := $(addprefix -I,$(INCDIR))

SRC_DIR = fstest/app src/IFS posix
OUT_DIR = out
OUT_DIRS := $(foreach sdir,$(SRC_DIR),$(OUT_DIR)/$(sdir))
SRC_FILES := $(wildcard $(foreach sdir,$(SRC_DIR),../$(sdir)/*.cpp))
OBJ_FILES := $(patsubst ../%.cpp,$(OUT_DIR)/%.o,$(SRC_FILES))

SPIFFS_SRC_DIR := ../../spiffs/src
SPIFFS_SRC_FILES := $(wildcard $(SPIFFS_SRC_DIR)/*.c)
OBJ_FILES += $(patsubst ../../spiffs/src/%.c,$(OUT_DIR)/spiffs/%.o,$(SPIFFS_SRC_FILES))
OUT_DIRS += $(OUT_DIR)/spiffs

FWFILES = $(OUT_DIR)\fwfiles.bin

all: $(OUT_DIR) $(FWFILES) bin

$(FWFILES):
	python ../fsbuild/fsbuild.py -i fsbuild.ini -o $@

bin: $(OUT_DIRS) fstest

$(OUT_DIR):
	$(Q) mkdir -p $@

$(OUT_DIRS):
	$(Q) mkdir -p $@

$(OUT_DIR)/%.o: ../%.cpp
	@echo "C++ $<"
	$(Q) $(HOST_CXX) $(INCDIR) $(CXXFLAGS) -c $< -o $@

$(OUT_DIR)/spiffs/%.o: ../../spiffs/src/%.c
	@echo "CC $<"
	$(Q) $(HOST_CC) $(INCDIR) $(CFLAGS) -c $< -o $@

fstest: $(OBJ_FILES)
	@echo "LD $@"
	$(Q) $(HOST_LD) -m32 -o $@ $^

clean:
	$(Q) rm -rf $(OUT_DIR)

