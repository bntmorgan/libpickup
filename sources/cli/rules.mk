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

sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)

TARGET					:= $(call SRC_2_BIN, $(d)/cli)
TARGETS 				+= $(TARGET)
OBJS_$(d)				:= $(call SRC_2_OBJ, $(d)/main.o $(d)/common/io.o \
	$(d)/common/log.o $(d)/db.o $(d)/common/http.o)

OBJECTS 				+= $(OBJS_$(d))

$(OBJS_$(d))		:  CC_FLAGS_TARGET	:= -I$(d) -I$(call SRC_2_OBJ, $(d)) \
	-I$(d)/common

$(TARGET)				:  LD_FLAGS_TARGET	:= -Lbinary/libcinder \
	-Lbinary/liboauth2webkit -lcinder -loauth2webkit `pkg-config sqlite3 --libs` \
	-lcurl
$(TARGET)				:  LD_OBJECTS	:= $(OBJS_$(d))
$(TARGET)				:  $(OBJS_$(d))

$(TARGET)				:  binary/libcinder/libcinder.so
$(TARGET)				:  binary/liboauth2webkit/liboauth2webkit.so

d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
