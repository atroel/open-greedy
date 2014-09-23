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

LDFLAGS-embed+=-lz

SDL:=$(abspath $(SROOT)/../SDL2-2.0.3)
CFLAGS-greedy+=-I$(SDL)/include
LDFLAGS-greedy+=-L$(SDL)/i686-w64-mingw32/lib -lSDL2 -lSDL2main

SDL_MIXER:=$(abspath $(SROOT)/../SDL2_mixer-2.0.0/i686-w64-mingw32)
CFLAGS-greedy+=-I$(SDL_MIXER)/include/SDL2
LDFLAGS-greedy+=-L$(SDL_MIXER)/lib -lSDL2_mixer

LDFLAGS-greedy+=-lopengl32

LDFLAGS-greedy+=$(abspath $(SROOT)/../../lib/libz.a)

greedy+=win32/
