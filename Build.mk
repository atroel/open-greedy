#
# Open Greedy - an open-source version of Edromel Studio's Greedy XP
#
# Copyright (C) 2014 Arnaud TROEL
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
CFLAGS-greedy+=-DBUILD=\"$(BUILD)\" -DVERSION=$(VERSION)

EXTRA_CFLAGS:=-Wall -Werror -pipe

BASICS:=$(abspath $(SROOT)/../basics)
EXTRA_CFLAGS+=-I$(BASICS)/include
EXTRA_LDFLAGS+=$(BASICS)/src/libb6.a

XMP_LITE:=$(abspath $(SROOT)/../libxmp-lite)
CFLAGS-greedy+=-I$(XMP_LITE)/include/libxmp-lite
LDFLAGS-greedy+=$(XMP_LITE)/lib/libxmp-lite.a
LDFLAGS-greedy+=-lm

ifeq ("$(notdir $(RROOT))","dbg")
EXTRA_CFLAGS+=-g3 -O0
else
EXTRA_CFLAGS+=-g0 -O3 -DNDEBUG
EXTRA_CFLAGS+=-fomit-frame-pointer -ffast-math -msse -mfpmath=sse
endif

bins+=greedy
greedy+=greedy.o gl/ sdl/ data/ core/ lib/

tools+=embed
embed+=embed.o lib/
