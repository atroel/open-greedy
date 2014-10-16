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

CFLAGS-greedy+=-F${HOME}/Library/Frameworks
CFLAGS-greedy+=-I${HOME}/Library/Frameworks/SDL2.framework/Headers
CFLAGS-greedy+=-I${HOME}/Library/Frameworks/SDL2_mixer.framework/Headers
LDFLAGS-greedy+=-F${HOME}/Library/Frameworks
LDFLAGS-greedy+=-framework SDL2
LDFLAGS-greedy+=-framework SDL2_mixer
LDFLAGS-greedy+=-framework OpenGL
LDFLAGS-greedy+=-framework CoreServices
LDFLAGS-greedy+=-lz
LDFLAGS-greedy+=-Wl,-rpath,@loader_path/../Frameworks

EXTRA_CFLAGS+=-I$(SROOT)/macos
EXTRA_LDFLAGS+=-rdynamic # for backtrace

greedy+=posix/ macos/
