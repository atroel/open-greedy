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

ifneq (,$(SDL))
SDL:=$(abspath $(SDL))
else
SDL:=$(abspath $(CURDIR)/../../SDL2-2.0.3)
ifeq (,$(wildcard $(SDL)))
$(warning Cannot find SDL2 in $(SDL))
SDL:=$(abspath $(CURDIR)/../SDL2-2.0.3)
endif
endif
ifeq (,$(wildcard $(SDL)))
$(error Cannot find SDL2 in $(SDL))
endif
cflags-greedy+=-I$(SDL)/include
ldflags-greedy+=-L$(SDL)/i686-w64-mingw32/lib -lSDL2 -lSDL2main

ifneq (,$(SDL_MIXER))
SDL_MIXER:=$(abspath $(SDL_MIXER))
else
SDL_MIXER:=$(abspath $(CURDIR)/../../SDL2_mixer-2.0.0/i686-w64-mingw32)
ifeq (,$(wildcard $(SDL_MIXER)))
$(warning Cannot find SDL2 mixer in $(SDL_MIXER))
SDL_MIXER:=$(abspath $(CURDIR)/../SDL2_mixer-2.0.0/i686-w64-mingw32)
endif
endif
ifeq (,$(wildcard $(SDL_MIXER)))
$(error Cannot find SDL2 mixer in $(SDL_MIXER))
endif
cflags-greedy+=-I$(SDL_MIXER)/include/SDL2
ldflags-greedy+=-L$(SDL_MIXER)/lib -lSDL2_mixer

ldflags-greedy+=-lopengl32

ifneq (,$(ZLIB))
ZLIB:=$(abspath $(ZLIB))
else
ZLIB:=$(abspath $(CURDIR)/../../../lib/libz.a)
ifeq (,$(wildcard $(ZLIB)))
$(warning Cannot find zlib in $(ZLIB))
ZLIB:=$(abspath $(CURDIR)/../../lib/libz.a)
endif
endif
ifeq (,$(wildcard $(ZLIB)))
$(error Cannot find zlib in $(ZLIB))
endif

ldflags-embed+=-lz
ldflags-greedy+=$(ZLIB)

cflags+=-I$(CURDIR)/win32

greedy+=win32/
