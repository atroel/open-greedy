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

#ifndef MOBILE_H
#define MOBILE_H

#include <b6/clock.h>
#include <b6/list.h>
#include "level.h"

struct mobile {
	const struct mobile_ops *ops;
	double speed; /* place/us */
	double delta;
	const struct b6_clock *clock;
	unsigned long long int timestamp_us; /* us */
	struct level *level;
	struct place *curr;
	struct place *next;
	enum direction direction;
	struct b6_dref moves[4];
	struct b6_list orders;
	double x, y;
	short int uturn;
	short int locked;
};

struct mobile_ops {
	void (*enter)(struct mobile*);
	void (*leave)(struct mobile*);
};

static inline void initialize_mobile(struct mobile *self,
				     const struct mobile_ops *ops,
				     const struct b6_clock *clock,
				     double speed)
{
	int i;
	b6_precond(speed > 0);
	self->ops = ops;
	self->clock = clock;
	self->speed = speed / 1e6;
	for (i = 0; i < b6_card_of(self->moves); i += 1)
		self->moves[i].ref[0] = NULL;
	b6_list_initialize(&self->orders);
}

static inline void lock_mobile(struct mobile *self) { self->locked = 1; }

static inline void unlock_mobile(struct mobile *self) { self->locked = 0; }

static inline double get_mobile_speed(const struct mobile *self)
{
	return self->speed * 1e6;
}

static inline void set_mobile_speed(struct mobile *self, double speed)
{
	self->speed = speed /= 1e6;
}

extern double update_mobile(struct mobile *self);

static inline int mobile_wants_to_go_to(const struct mobile *self,
					enum direction d)
{
	return !!self->moves[d].ref[0];
}

static inline void cancel_mobile_move(struct mobile *self, enum direction d)
{
	if (mobile_wants_to_go_to(self, d)) {
		struct b6_dref *move = &self->moves[d];
		b6_list_del(move);
		move->ref[0] = NULL;
	}
}

static inline void cancel_all_mobile_moves(struct mobile *self)
{
	cancel_mobile_move(self, LEVEL_E);
	cancel_mobile_move(self, LEVEL_W);
	cancel_mobile_move(self, LEVEL_S);
	cancel_mobile_move(self, LEVEL_N);
}

static inline void submit_mobile_move(struct mobile *self, enum direction d)
{
	cancel_mobile_move(self, d);
	b6_list_add_first(&self->orders, &self->moves[d]);
}

static inline void reset_mobile(struct mobile *self, struct level *level,
				struct place *place, int uturn)
{
	self->level = level;
	self->curr = place;
	self->next = NULL;
	self->direction = LEVEL_E;
	self->delta = 0;
	self->uturn = uturn;
	self->locked = 0;
	self->timestamp_us = b6_get_clock_time(self->clock);
	cancel_all_mobile_moves(self);
}

static inline struct place *get_mobile_place(const struct mobile *self)
{
	return self->curr;
}

static inline int mobile_is_stopped(const struct mobile *self)
{
	return !self->next;
}

static inline enum direction get_mobile_direction(const struct mobile *self)
{
	b6_precond(!mobile_is_stopped(self));
	return self->direction;
}

static inline int mobile_has_orders(const struct mobile *self)
{
	return !b6_list_empty(&self->orders);
}

#endif /* MOBILE_H */
