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

cflags-greedy+=-F${HOME}/Library/Frameworks
cflags-greedy+=-I${HOME}/Library/Frameworks/SDL2.framework/Headers
cflags-greedy+=-I${HOME}/Library/Frameworks/SDL2_mixer.framework/Headers
ldflags-greedy+=-F${HOME}/Library/Frameworks
ldflags-greedy+=-framework SDL2
ldflags-greedy+=-framework SDL2_mixer
ldflags-greedy+=-framework OpenGL
ldflags-greedy+=-framework CoreServices
ldflags-greedy+=-Wl,-rpath,@loader_path/../Frameworks

cflags+=-I$(CURDIR)/macos
cflags+=-DRO_DIR=\"../Resources/data\"
ldflags+=-rdynamic # for backtrace
ldflags+=-lz

greedy+=posix/
