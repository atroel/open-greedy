/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014-2015 Arnaud TROEL
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

#include "menu_controller.h"

#include "menu.h"

static void menu_controller_handle_key_pressed(struct menu *menu,
					       enum controller_key key)
{
	switch (key) {
	case CTRLK_UP:
		previous_menu_entry(menu);
		break;
	case CTRLK_DOWN:
		next_menu_entry(menu);
		break;
	case CTRLK_RETURN:
	case CTRLK_ENTER:
		select_menu_entry(menu);
		break;
	case CTRLK_ESCAPE:
		quit_menu(menu);
		break;
	default:
		break;
	}
}

static void menu_controller_on_key_pressed(struct controller_observer *observer,
					   enum controller_key key)
{
	struct menu_controller *self =
		b6_cast_of(observer, struct menu_controller,
			   controller_observer);
	menu_controller_handle_key_pressed(self->menu, key);
}

const struct controller_observer_ops __menu_controller_observer_ops = {
	.on_key_pressed = menu_controller_on_key_pressed,
};
