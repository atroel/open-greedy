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

#ifndef GHOSTS_H
#define GHOSTS_H

#include "level.h"
#include "mobile.h"

struct game;
struct clock;

enum ghost_state {
	GHOST_OUT,
	GHOST_HUNTER,
	GHOST_AFRAID,
	GHOST_ZOMBIE,
	GHOST_FROZEN,
};

struct ghost {
	struct mobile mobile;
	enum ghost_state state;
	struct ghost_strategy *current_strategy;
	struct ghost_strategy *default_strategy;
};

extern void initialize_ghost(struct ghost *self, int n, float speed,
			     const struct b6_clock*);

static inline void reset_ghost(struct ghost *self, struct level *level)
{
	reset_mobile(&self->mobile, level, level->ghosts_home, 1);
	self->state = GHOST_OUT;
}

extern void set_ghost_state(struct ghost *self, enum ghost_state state);

static inline enum ghost_state get_ghost_state(const struct ghost *self)
{
	return self->state;
}

static inline int ghost_is_alive(const struct ghost *ghost)
{
	enum ghost_state state = get_ghost_state(ghost);
	return state != GHOST_OUT && state != GHOST_ZOMBIE;
}

extern void initialize_ghost_strategies(struct game *game);

#endif /* GHOSTS_H */
