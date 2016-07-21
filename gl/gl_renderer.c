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

#include "gl_renderer.h"

#include <b6/array.h>
#include <b6/cmdline.h>
#include <b6/pool.h>

#include "core/renderer.h"
#include "gl_utils.h"

static int gl_pot = 0;
b6_flag(gl_pot, bool);

struct gl_texture {
	struct renderer_texture texture;
	GLuint id;
	double w, h;
};

static struct gl_texture *to_gl_texture(struct renderer_texture *up)
{
	return b6_cast_of(up, struct gl_texture, texture);
}

struct gl_tile {
	struct renderer_tile tile;
};

static struct gl_tile *to_gl_tile(struct renderer_tile *up)
{
	return b6_cast_of(up, struct gl_tile, tile);
}

struct gl_base {
	struct renderer_base base;
};

static struct gl_base *to_gl_base(struct renderer_base *up)
{
	return b6_cast_of(up, struct gl_base, base);
}

static struct gl_renderer *to_gl_renderer(struct renderer *up)
{
	return b6_cast_of(up, struct gl_renderer, renderer);
}

struct renderer_base *get_gl_root(struct renderer *up)
{
	struct gl_renderer *self = to_gl_renderer(up);
	return &self->root;
}

static void delete_gl_base(struct renderer_base *up)
{
	struct gl_base *self = to_gl_base(up);
	b6_deallocate(to_gl_renderer(up->renderer)->base_allocator, self);
}

static struct renderer_base *new_gl_base(struct renderer *up,
					 struct renderer_base *parent,
					 const char *name, double x, double y)
{
	static const struct renderer_base_ops ops = { .dtor = delete_gl_base, };
	struct gl_renderer *renderer = to_gl_renderer(up);
	struct gl_base *self = b6_allocate(renderer->base_allocator,
					     sizeof(*self));
	if (!self)
		return NULL;
	__setup_renderer_base(&self->base, parent, name, x, y, &ops);
	return &self->base;
}

static void delete_gl_tile(struct renderer_tile *up)
{
	struct gl_tile *self = to_gl_tile(up);
	struct gl_renderer *renderer = to_gl_renderer(up->renderer);
	b6_deallocate(renderer->tile_allocator, self);
	renderer->dirty = 1;
}

struct renderer_tile *new_gl_tile(struct renderer *up,
				  struct renderer_base *base,
				  double x, double y, double w, double h,
				  struct renderer_texture *texture)
{
	static const struct renderer_tile_ops ops = { .dtor = delete_gl_tile, };
	struct gl_renderer *renderer = to_gl_renderer(up);
	struct gl_tile *self = b6_allocate(renderer->tile_allocator,
					      sizeof(*self));
	if (!self)
		return NULL;
	__setup_renderer_tile(&self->tile, base, x, y, w, h, texture, &ops);
	renderer->dirty = 1;
	return &self->tile;
}

static void delete_gl_texture(struct renderer_texture *up)
{
	struct gl_texture *self = to_gl_texture(up);
	gl_call(glDeleteTextures(1, &self->id));
	b6_deallocate(to_gl_renderer(up->renderer)->texture_allocator, self);
}

static void update_gl_texture(struct renderer_texture *up,
			      const struct rgba *rgba)
{
	struct gl_texture *self = to_gl_texture(up);
	make_gl_texture(self->id, rgba);
	self->w = 1.;
	self->h = 1.;
}

static unsigned long int to_pot(unsigned short int n)
{
	if (b6_is_apot(n))
		return n;
	n |= n >> 8;
	n |= n >> 4;
	n |= n >> 2;
	n |= n >> 1;
	return n + 1;
}

static void update_gl_texture_pot(struct renderer_texture *up,
				  const struct rgba *rgba)
{
	struct gl_texture *self = to_gl_texture(up);
	unsigned long int w = to_pot(rgba->w), h = to_pot(rgba->h);
	struct rgba temp;
	if (w == rgba->w && h == rgba->h) {
		update_gl_texture(up, rgba);
		return;
	}
	initialize_rgba(&temp, w, h);
	copy_rgba(rgba, 0, 0, rgba->w, rgba->h, &temp, 0, 0);
	self->w = (double)rgba->w / w;
	self->h = (double)rgba->h / h;
	make_gl_texture(self->id, &temp);
	finalize_rgba(&temp);
}

static struct renderer_texture *new_gl_texture(struct renderer *up,
					       const struct rgba *rgba)
{
	static const struct renderer_texture_ops ops = {
		.update = update_gl_texture,
		.dtor = delete_gl_texture,
	};
	static const struct renderer_texture_ops ops_pot = {
		.update = update_gl_texture_pot,
		.dtor = delete_gl_texture,
	};
	struct gl_renderer *gl_renderer = to_gl_renderer(up);
	struct gl_texture *self = b6_allocate(gl_renderer->texture_allocator,
					      sizeof(*self));
	if (!self)
		return NULL;
	self->texture.ops = gl_pot ? &ops_pot : &ops;
	gl_call(glGenTextures(1, &self->id));
	self->texture.ops->update(&self->texture, rgba);
	return &self->texture;
}

static void gl_prerender_tiles(struct renderer_base *self)
{
	struct gl_buffer *gl_buffer = to_gl_renderer(self->renderer)->gl_buffer;
	struct b6_dref *dref;
	for (dref = b6_list_first(&self->tiles);
	     dref != b6_list_tail(&self->tiles);
	     dref = b6_list_walk(dref, B6_NEXT)) {
		struct renderer_tile *tile =
			b6_cast_of(dref, struct renderer_tile, dref);
		double x0 = tile->x;
		double y0 = tile->y;
		double x1 = x0 + tile->w;
		double y1 = y0 + tile->h;
		float w = 1., h = 1.;
		if (tile->texture) {
			struct gl_texture *tx = to_gl_texture(tile->texture);
			w = tx->w;
			h = tx->h;
		}
		append_gl_buffer(gl_buffer, 0., 0., w, h, x0, y0, x1, y1);
	}
}

static void gl_prerender_base(struct renderer_base *self)
{
	struct b6_dref *dref;
	gl_prerender_tiles(self);
	for (dref = b6_list_first(&self->bases);
	     dref != b6_list_tail(&self->bases);
	     dref = b6_list_walk(dref, B6_NEXT))
		gl_prerender_base(b6_cast_of(dref, struct renderer_base, dref));
}

static void gl_render_tiles(struct gl_renderer *self,
			    struct renderer_base *base,
			    unsigned long int *index, int visible)
{
	struct b6_dref *dref;
	struct renderer_texture *texture = NULL;
	unsigned long int local = 0;
	if (!visible) {
		for (dref = b6_list_first(&base->tiles);
		     dref != b6_list_tail(&base->tiles);
		     dref = b6_list_walk(dref, B6_NEXT))
			*index += 6;
		return;
	}
	for (dref = b6_list_first(&base->tiles);
	     dref != b6_list_tail(&base->tiles);
	     dref = b6_list_walk(dref, B6_NEXT)) {
		struct renderer_tile *tile =
			b6_cast_of(dref, struct renderer_tile, dref);
		if (tile->texture != texture) {
			if (texture != NULL) {
				self->draw_count += 1;
				bind_gl_texture(to_gl_texture(texture)->id);
				gl_call(glDrawArrays(GL_TRIANGLES, local,
						     *index - local));
			}
			texture = tile->texture;
			local = *index;
		}
		*index += 6;
	}
	if (texture != NULL) {
		self->draw_count += 1;
		bind_gl_texture(to_gl_texture(texture)->id);
		gl_call(glDrawArrays(GL_TRIANGLES, local, *index - local));
	}
}

static void gl_render_base(struct gl_renderer *self, struct renderer_base *up,
			   unsigned long int *index, int visible)
{
	struct b6_dref *dref;
	visible &= up->visible;
	gl_call(glPushMatrix());
	gl_call(glTranslatef(up->x, up->y, 0));
	gl_render_tiles(self, up, index, visible);
	for (dref = b6_list_first(&up->bases);
	     dref != b6_list_tail(&up->bases);
	     dref = b6_list_walk(dref, B6_NEXT))
		gl_render_base(self,
			       b6_cast_of(dref, struct renderer_base, dref),
			       index, visible);
	gl_call(glPopMatrix());
}

static void gl_render(struct renderer *up)
{
	struct gl_renderer *self = to_gl_renderer(up);
	unsigned long int index = 0;
	if (self->dirty) {
		unbind_gl_texture();
		clear_gl_buffer(self->gl_buffer);
		gl_prerender_base(&self->root);
		push_gl_buffer(self->gl_buffer);
		self->dirty = 0;
	}
	gl_call(glLoadIdentity());
	gl_call(glClear(GL_COLOR_BUFFER_BIT|
			GL_DEPTH_BUFFER_BIT|
			GL_STENCIL_BUFFER_BIT));
	gl_call(glColor3f(self->dim, self->dim, self->dim));
	self->draw_count = 0;
	gl_render_base(self, &self->root, &index, 1);
}

static void gl_resize(struct renderer *up)
{
	double wi = up->internal_width;
	double hi = up->internal_height;
	double we = up->external_width;
	double he = up->external_height;
	if (we <= 0 || he <= 0 || wi <= 0 || hi <= 0)
		return;
	if (we * hi > he * wi) {
		double width = wi / hi * he;
		gl_call(glViewport((we - width) / 2, 0, width, he));
	} else {
		double height = hi / wi * we;
		gl_call(glViewport(0, (he - height) / 2, we, height));
	}
	gl_call(glMatrixMode(GL_PROJECTION));
	gl_call(glLoadIdentity());
	gl_call(glOrtho(0, wi, hi, 0, 100, -100));
	gl_call(glMatrixMode(GL_MODELVIEW));
}

static void gl_start(struct renderer *up)
{
	gl_call(glClearColor(.0f, .0f, .0f, 1.f));
	gl_call(glClear(GL_COLOR_BUFFER_BIT|
			GL_DEPTH_BUFFER_BIT|
			GL_STENCIL_BUFFER_BIT));
	gl_call(glDisable(GL_DEPTH_TEST));
	gl_call(glDepthMask(GL_TRUE));
	gl_call(glColor4f(1.f, 1.f, 1.f, 1.f));
	gl_call(glEnable(GL_TEXTURE_2D));
	gl_call(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	gl_call(glEnableClientState(GL_VERTEX_ARRAY));
	gl_call(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
	gl_call(glEnable(GL_ALPHA_TEST));
	gl_call(glEnable(GL_BLEND));
}

static void gl_stop(struct renderer *up)
{
	gl_call(glDisable(GL_BLEND));
	gl_call(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
	gl_call(glDisableClientState(GL_VERTEX_ARRAY));
}

static void gl_dim(struct renderer *up, float value)
{
	struct gl_renderer *self = to_gl_renderer(up);
	self->dim = value;
}

int open_gl_renderer(struct gl_renderer *self)
{
	static const struct renderer_ops ops = {
		.get_root = get_gl_root,
		.new_base = new_gl_base,
		.new_tile = new_gl_tile,
		.new_texture = new_gl_texture,
		.start = gl_start,
		.stop = gl_stop,
		.resize = gl_resize,
		.render = gl_render,
		.dim = gl_dim,
	};
	__setup_renderer(&self->renderer, &ops);
	__setup_renderer_base(&self->root, NULL, "gl_root", 0, 0, NULL);
	self->root.renderer = &self->renderer;
	self->dim = 1.f;
	if (!initialize_gl_srv_buffer(&self->srv_buffer)) {
		log_i(_s("using remote gl buffer"));
		self->gl_buffer = &self->srv_buffer.gl_buffer;
	} else if (!initialize_gl_cli_buffer(&self->cli_buffer)) {
		log_i(_s("using local gl buffer"));
		self->gl_buffer = &self->cli_buffer.gl_buffer;
	} else
		return -1;
	b6_reset_fixed_allocator(&self->allocator, self->buffer,
				 sizeof(self->buffer));
	b6_pool_initialize(&self->pool, &self->allocator.allocator, 1024,
			   sizeof(self->buffer));
	b6_pool_initialize(&self->texture_pool, &self->pool.parent,
			   sizeof(struct gl_texture), 1024);
	b6_pool_initialize(&self->tile_pool, &self->pool.parent,
			   sizeof(struct gl_tile), 1024);
	b6_pool_initialize(&self->base_pool, &self->pool.parent,
			   sizeof(struct gl_base), 1024);
	self->texture_allocator = &self->texture_pool.parent;
	self->tile_allocator = &self->tile_pool.parent;
	self->base_allocator = &self->base_pool.parent;
	self->dirty = 1;
	return 0;
}

void close_gl_renderer(struct gl_renderer *self)
{
	b6_pool_finalize(&self->base_pool);
	b6_pool_finalize(&self->tile_pool);
	b6_pool_finalize(&self->texture_pool);
	b6_pool_finalize(&self->pool);
	if (self->gl_buffer == &self->cli_buffer.gl_buffer)
		finalize_gl_cli_buffer(&self->cli_buffer);
	else if (self->gl_buffer == &self->srv_buffer.gl_buffer)
		finalize_gl_srv_buffer(&self->srv_buffer);
}
