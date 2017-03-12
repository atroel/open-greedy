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

libs+=lib.a
lib.a:=utils.o
lib.a+=greedy_skin.o
lib.a+=default_skin.o
lib.a+=default_font.tga.data.o default_game.tga.data.o
lib.a+=default_level_01.lev.data.o default_level_02.lev.data.o \
	default_level_03.lev.data.o default_level_04.lev.data.o \
	default_level_05.lev.data.o
