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

#include "b6/clock.h"
#include "mobile.h"

static void do_enter(struct mobile *self)
{
	if (self->ops->enter)
		self->ops->enter(self);
}

static void do_leave(struct mobile* self)
{
	if (self->ops->leave)
		self->ops->leave(self);
}

static enum direction get_order_direction(struct mobile *m, struct b6_dref *r)
{
	return r - m->moves;
}

static void enter(struct mobile *self)
{
	b6_assert(self->next);
	self->curr = self->next;
	do_enter(self);
}

/* The mobile stops if it has no active commands. Otherwise, its tries moving
 * according to the first command, the direction it was moving towards or
 * eventually falls back to the rest of the command backlog. If a command
 * matches, it becomes the first command.
 */
static int leave(struct mobile *self)
{
	enum direction direction;
	struct b6_dref *order = b6_list_first(&self->orders);
	if (order == b6_list_tail(&self->orders))
		goto no_more_orders;
	direction = get_order_direction(self, order);
	if ((self->next = place_neighbor(self->level, self->curr, direction))) {
		self->direction = direction;
		do_leave(self);
		return 1;
	}
	if ((self->next = place_neighbor(self->level, self->curr,
					 self->direction))) {
		do_leave(self);
		return 1;
	}
	for (order = b6_list_walk(order, B6_NEXT);
	     order != b6_list_tail(&self->orders);
	     order = b6_list_walk(order, B6_NEXT)) {
		direction = get_order_direction(self, order);
		if ((self->next = place_neighbor(self->level, self->curr,
						 direction))) {
			cancel_mobile_move(self, direction);
			submit_mobile_move(self, direction);
			self->direction = direction;
			do_leave(self);
			return 1;
		}
	}
no_more_orders:
	self->next = NULL;
	return 0;
}

double update_mobile(struct mobile *self)
{
	enum direction direction;
	struct b6_dref *order;
	struct place *place;
	int x, y;
	double distance = 0;
	unsigned long long int now = b6_get_clock_time(self->clock);
	unsigned long long int dt = now - self->timestamp_us;
	self->timestamp_us = now;
	if (self->locked)
		goto done;
	if (mobile_is_stopped(self)) {
		leave(self);
		goto done;
	}
	self->delta += dt * self->speed;
	if (self->delta > 10.)
		self->delta = 10.;
	while (self->delta >= 1.) {
		self->delta -= 1;
		distance += 1;
		enter(self);
		if (!leave(self)) {
			self->delta = 0.;
			break;
		}
	}
	order = b6_list_first(&self->orders);
	if (order == b6_list_tail(&self->orders))
		goto done;
	direction = get_order_direction(self, order);
	if (!is_opposite(self->direction, direction))
		goto done;
	if (!self->uturn)
		goto done;
	place = self->curr;
	self->curr = self->next;
	self->next = place;
	self->direction = direction;
	self->delta = 1 - self->delta;
done:
	place_location(self->level, self->curr, &x, &y);
	self->x = x;
	self->y = y;
	switch (self->direction) {
	case LEVEL_N: self->y -= self->delta; break;
	case LEVEL_S: self->y += self->delta; break;
	case LEVEL_W: self->x -= self->delta; break;
	case LEVEL_E: self->x += self->delta; break;
	}
	return distance;
}
