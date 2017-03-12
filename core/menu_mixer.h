/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014-2017 Arnaud TROEL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MENU_MIXER_H
#define MENU_MIXER_H

#include "menu.h"

struct menu_mixer {
	struct menu_observer menu_observer;
	struct menu *menu;
	struct mixer *mixer;
	int music;
};

extern int initialize_menu_mixer(struct menu_mixer*, struct menu*,
				 const char *skin, struct mixer*);

extern void finalize_menu_mixer(struct menu_mixer*);

#endif /* MENU_MIXER_H */
