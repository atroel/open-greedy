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

#ifndef LINEAR_H
#define LINEAR_H

#include <b6/clock.h>
#include "lib/log.h"

struct linear {
	const struct b6_clock *clock;
	double actual, target, speed;
	unsigned long long int time;
};

static inline void set_linear_target(struct linear *self, double target)
{
	self->target = target;
	self->time = b6_get_clock_time(self->clock);
}

static inline void stop_linear(struct linear *self)
{
	set_linear_target(self, self->actual);
}

static inline int linear_is_stopped(const struct linear *self)
{
	return self->actual == self->target;
}

static inline void reset_linear(struct linear *self,
				double actual, double target, double speed)
{
	self->actual = actual;
	self->speed = speed;
	set_linear_target(self, target);
}

static inline void setup_linear(struct linear *self,
				const struct b6_clock *clock,
				double actual, double target, double speed)
{
	self->clock = clock;
	reset_linear(self, actual, target, speed);
}

static inline double update_linear(struct linear *self)
{
	unsigned long long int time = b6_get_clock_time(self->clock);
	unsigned long long int elapsed = time - self->time;
	self->time = time;
	if (self->actual < self->target) {
		self->actual += elapsed * self->speed;
		if (self->actual > self->target)
			self->actual = self->target;
	} else {
		self->actual -= elapsed * self->speed;
		if (self->actual < self->target)
			self->actual = self->target;
	}
	return self->actual;
}

#endif /* LINEAR_H */
