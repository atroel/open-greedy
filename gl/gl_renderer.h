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

#ifndef GL_RENDERER_H
#define GL_RENDERER_H

#include "core/renderer.h"
#include "gl_utils.h"

#include <b6/pool.h>

struct gl_renderer {
	struct renderer renderer;
	struct renderer_base root;
	float dim;
	struct b6_allocator *texture_allocator;
	struct b6_allocator *tile_allocator;
	struct b6_allocator *base_allocator;
	unsigned char buffer[65536];
	struct b6_fixed_allocator allocator;
	struct b6_pool pool;
	struct b6_pool texture_pool;
	struct b6_pool tile_pool;
	struct b6_pool base_pool;
	int dirty;
	struct gl_cli_buffer cli_buffer;
	struct gl_srv_buffer srv_buffer;
	struct gl_buffer *gl_buffer;
	int draw_count;
};

extern int open_gl_renderer(struct gl_renderer *self);

void close_gl_renderer(struct gl_renderer *self);

#endif /* GL_RENDERER_H */
