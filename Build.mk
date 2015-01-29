#
# Open Greedy - an open-source version of Edromel Studio's Greedy XP
#
# Copyright (C) 2014-2015 Arnaud TROEL
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

BUILD?=$(shell date -u +'%Y%m%d%H%M%S')
VERSION:=$(shell cat $(SROOT)/VERSION)

ifeq ($(PLATFORM),)
OS=$(shell uname -s)
ifeq ($(OS),Linux)
export PLATFORM:=linux
else
ifeq ($(OS),Darwin)
export PLATFORM:=macos
else
export PLATFORM:=win32
endif
endif
endif

ifeq ("$R","dbg")
cflags+=-g3 -O0
endif
ifeq ("$R","opt")
cflags+=-g0 -O3 -DNDEBUG -ffast-math -msse -mfpmath=sse
endif
cflags+=-Wall -Werror -pipe
ldflags+=-lz
cppflags+=-I$(CURDIR)

bins+=greedy
greedy+=greedy.o gl/ sdl/ data/ core/ lib/

tools+=embed
embed+=embed.o lib/

BASICS?=$(abspath $(SROOT)/../basics)
cppflags+=-I$(BASICS)/include
ldflags+=$(BASICS)/src/libb6.a

XMP_LITE?=$(abspath $(SROOT)/../libxmp-lite)
cppflags-greedy+=-I$(XMP_LITE)/include/libxmp-lite
ldflags-greedy+=$(XMP_LITE)/lib/libxmp-lite.a

cppflags-greedy+=-DBUILD=\"$(BUILD)\"
cppflags-greedy+=-DVERSION=\"$(VERSION)\"
cppflags-greedy+=-DPLATFORM=\"$(PLATFORM)\"

ldflags-greedy+=-lm

-include Build-$(PLATFORM).mk
