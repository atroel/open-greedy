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

#ifndef FADE_IO_H
#define FADE_IO_H

#include "linear.h"
#include "renderer.h"

struct fade_io {
	struct renderer_observer renderer_observer;
	struct renderer *renderer;
	struct linear linear;
};

static inline void set_fade_io_target(struct fade_io *self, float target)
{
	set_linear_target(&self->linear, target);
}

static inline void stop_fade_io(struct fade_io *self)
{
	stop_linear(&self->linear);
}

static inline int fade_io_is_stopped(const struct fade_io *self)
{
	return linear_is_stopped(&self->linear);
}

static inline void reset_fade_io(struct fade_io *self,
				 float actual, float target, float speed)
{
	reset_linear(&self->linear, actual, target, speed);
}

extern void on_render_fade_io_internal(struct renderer_observer*);

static inline void initialize_fade_io(struct fade_io *self,
				      const char *debug_name,
				      struct renderer *renderer,
				      const struct b6_clock *clock,
				      float actual, float target, float speed)
{
	static const struct renderer_observer_ops ops = {
		.on_render = on_render_fade_io_internal,
	};
	self->renderer = renderer;
	setup_linear(&self->linear, clock, actual, target, speed);
	add_renderer_observer(self->renderer,
			      setup_renderer_observer(&self->renderer_observer,
						      debug_name, &ops));
}

static inline void finalize_fade_io(struct fade_io *self)
{
	del_renderer_observer(&self->renderer_observer);
}

#endif /* FADE_IO_H */
