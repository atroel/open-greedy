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

#include "renderer.h"

#define __notify_renderer_observers(_self, _op, _args...) \
	b6_notify_observers(&(_self)->observers, renderer_observer, _op, \
			   ##_args)

B6_REGISTRY_DEFINE(__base_registry);

void destroy_renderer_base_tiles(struct renderer_base *self)
{
	struct b6_dref *dref = b6_list_first(&self->tiles);
	while (dref != b6_list_tail(&self->tiles)) {
		struct renderer_tile *s =
			b6_cast_of(dref, struct renderer_tile, dref);
		dref = b6_list_walk(dref, B6_NEXT);
		destroy_renderer_tile(s);
	}
}

void destroy_renderer_base_children(struct renderer_base *self)
{
	struct b6_dref *dref = b6_list_first(&self->bases);
	while (dref != b6_list_tail(&self->bases)) {
		struct renderer_base *w =
			b6_cast_of(dref, struct renderer_base, dref);
		dref = b6_list_walk(dref, B6_NEXT);
		destroy_renderer_base(w);
	}
}

void show_renderer(struct renderer *self)
{
	__notify_renderer_observers(self, on_render);
	self->ops->render(self);
}
