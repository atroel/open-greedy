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

#ifndef PACMAN_H
#define PACMAN_H

#include "level.h"
#include "mobile.h"

struct pacman {
	struct mobile mobile;
	unsigned int score;
	float booster;
	int lifes;
	int shields;
	unsigned int jewels;
};

extern void initialize_pacman(struct pacman *self, const struct b6_clock *clock,
			      float speed, float boost);

static inline void reset_pacman(struct pacman *self, struct level *level)
{
	reset_mobile(&self->mobile, level, level->pacman_home, 0);
}

static inline void reload_booster(struct pacman *self, float count, float limit)
{
	b6_assert(count > 0);
	self->booster += count;
	if (self->booster >= limit)
		self->booster = limit;
}

static inline void charge_booster(struct pacman *self, float count)
{
	b6_assert(count > 0);
	self->booster -= count;
	if (self->booster <= 0)
		self->booster = 0;
}

#endif /* PACMAN_H */
