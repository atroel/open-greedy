/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014-2016 Arnaud TROEL
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

#include "pacman.h"
#include "items.h"

static void pacman_enter(struct mobile *mobile)
{
	pacman_visits_item(mobile, mobile->curr->item);
}

void initialize_pacman(struct pacman *self, const struct b6_clock *clock,
		       float speed, float booster)
{
	static const struct mobile_ops ops = { .enter = pacman_enter, };
	initialize_mobile(&self->mobile, &ops, clock, speed);
	self->score = 0;
	self->booster = booster;
	self->lifes = 2;
	self->jewels = 0;
	self->shields = 0;
}
