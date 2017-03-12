#
# Open Greedy - an open-source version of Edromel Studio's Greedy XP
#
# Copyright (C) 2014-2017 Arnaud TROEL
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

cflags-sdl2?=$(shell sdl2-config --cflags)
ldflags-sdl2?=$(shell sdl2-config --libs)

cflags-greedy+=$(cflags-sdl2)
ldflags-greedy+=$(ldflags-sdl2)
ldflags-greedy+=-lGL
ldflags-greedy+=-lSDL2
ldflags-greedy+=-lSDL2_mixer

cflags+=-I$(CURDIR)/linux
cflags+=-DRO_DIR=\"/usr/share/games/opengreedy/data,data\"
ldflags+=-rdynamic # for backtrace
ldflags+=-lz

greedy+=linux/ posix/
