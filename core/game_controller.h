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

#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "controller.h"

struct game_controller {
	struct controller_observer controller_observer;
	struct controller *controller;
	struct game *game;
};

extern const struct controller_observer_ops __game_controller_observer_ops;

static inline void initialize_game_controller(struct game_controller *self,
					      struct game *game,
					      struct controller *controller)
{
	self->controller = controller;
	self->game = game;
	add_controller_observer(controller, setup_controller_observer(
			&self->controller_observer,
			&__game_controller_observer_ops));
}

static inline void finalize_game_controller(struct game_controller *self)
{
	del_controller_observer(&self->controller_observer);
}

#endif /* GAME_CONTROLLER_H */
