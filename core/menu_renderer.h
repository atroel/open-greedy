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

#ifndef MENU_RENDERER_H
#define MENU_RENDERER_H

#include "linear.h"
#include "menu.h"
#include "toolkit.h"
#include "renderer.h"

struct menu_entry_renderer {
	struct renderer_observer renderer_observer;
	const struct b6_clock *clock;
	struct toolkit_label label[2];
};

struct menu_renderer_image {
	struct renderer_observer renderer_observer;
	struct renderer_base *base;
	struct renderer_tile *tile[2];
	struct linear linear;
	const struct b6_clock *clock;
};

struct menu_renderer {
	struct renderer_observer renderer_observer;
	struct menu_observer menu_observer;
	struct renderer *renderer;
	const struct b6_clock *clock;
	struct menu *menu;
	struct fixed_font normal_font;
	struct fixed_font bright_font;
	struct renderer_tile *background;
	struct menu_renderer_image title;
	struct menu_renderer_image pacman;
	struct menu_renderer_image ghost;
};

extern int initialize_menu_renderer(struct menu_renderer *self,
				    struct renderer *renderer,
				    const struct b6_clock *clock,
				    const char *skin_id);

extern void finalize_menu_renderer(struct menu_renderer *self);

extern void open_menu_renderer(struct menu_renderer *self, struct menu *menu);

extern void close_menu_renderer(struct menu_renderer *self);

extern void update_menu_renderer(struct menu_renderer *self);

#endif /* MENU_RENDERER_H */
