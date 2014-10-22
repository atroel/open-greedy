/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014 Arnaud TROEL
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

#include "controller.h"

void __notify_controller_key_pressed(struct controller *self,
				     enum controller_key key)
{
	b6_notify_observers(&self->observers, controller_observer,
			    on_key_pressed, key);
}

void __notify_controller_key_released(struct controller *self,
				      enum controller_key key)
{
	b6_notify_observers(&self->observers, controller_observer,
			    on_key_released, key);
}

void __notify_controller_text_input(struct controller *self,
				    unsigned short int unicode)
{
	b6_notify_observers(&self->observers, controller_observer,
			    on_text_input, unicode);
}

void __notify_controller_quit(struct controller *self)
{
	b6_notify_observers(&self->observers, controller_observer, on_quit);
}

void __notify_controller_focus_in(struct controller *self)
{
	b6_notify_observers(&self->observers, controller_observer,
			    on_focus_in);
}

void __notify_controller_focus_out(struct controller *self)
{
	b6_notify_observers(&self->observers, controller_observer,
			    on_focus_out);
}
