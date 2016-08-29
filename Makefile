# Copyright (C) 2016  Benoît Morgan
#
# This file is part of libcinder.
#
# libcinder is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# libcinder is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with libcinder.  If not, see <http://www.gnu.org/licenses/>.

CC						:= gcc
LD						:= ld
BUILD_DIR			:= build
BINARY_DIR		:= binary

CC_FLAGS_ALL		:= -Wall -Werror -Werror -Isources/include

LD_FLAGS_ALL		:= -Lbinary/libcinder

define SRC_2_OBJ
  $(foreach src,$(1),$(patsubst sources/%,$(BUILD_DIR)/%,$(src)))
endef

define SRC_2_BIN
  $(foreach src,$(1),$(patsubst sources/%,$(BINARY_DIR)/%,$(src)))
endef

all: targets

# Overriden in rules.mk
TARGETS :=
OBJECTS :=

dir	:= sources
include	$(dir)/rules.mk

$(BINARY_DIR)/%:
	@echo "[LD] $@"
	@mkdir -p $(dir $@)
	@$(CC) -o $@ $(LD_OBJECTS) $(LD_FLAGS_ALL) $(LD_FLAGS_TARGET)

$(BINARY_DIR)/%.so:
	@echo "[LD] $@"
	@mkdir -p $(dir $@)
	@$(LD) -shared -o $@ $(LD_OBJECTS) $(LD_FLAGS_TARGET)

$(BUILD_DIR)/%.o: sources/%.c
	@echo "[CC] $< -> $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CC_FLAGS_ALL) $(CC_FLAGS_TARGET) -o $@ -c $<

targets: $(TARGETS)

clean:
	@rm -fr $(BUILD_DIR) $(BINARY_DIR)

info:
	@echo Targets [$(TARGETS)]
	@echo Objects [$(OBJECTS)]

run: all
	LD_LIBRARY_PATH=binary/libcinder/ ./binary/sample/sample

# Remove default rulez
.SUFFIXES:
