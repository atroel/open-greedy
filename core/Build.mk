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

libs+=lib.a
lib.a:=controller.o game.o game_controller.o game_mixer.o game_renderer.o \
	ghosts.o items.o level.o menu.o menu_controller.o menu_mixer.o \
	menu_renderer.o mixer.o mobile.o pacman.o renderer.o rgba.o data.o \
	toolkit.o engine.o game_phase.o menu_phase.o hall_of_fame.o \
	hall_of_fame_phase.o console.o fade_io.o credits_phase.o env.o
lib.a+=json.o lang.json.data.o
