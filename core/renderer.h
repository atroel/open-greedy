/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014-2015 Arnaud TROEL
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

#ifndef RENDERER_H
#define RENDERER_H

#include <b6/deque.h>
#include <b6/list.h>
#include <b6/observer.h>
#include <b6/registry.h>
#include <b6/utils.h>
#include "lib/log.h"
#include "rgba.h"

struct renderer_base {
	struct b6_dref dref;
	const struct renderer_base_ops *ops;
	struct renderer *renderer;
	struct renderer_base *parent;
	double x, y;
	int visible;
	struct b6_list bases;
	struct b6_list tiles;
	const char *name;
	void *userdata;
};

struct renderer_base_ops {
	void (*dtor)(struct renderer_base*);
};

static inline void __setup_renderer_base(struct renderer_base *self,
					 struct renderer_base *parent,
					 const char *name, double x, double y,
					 const struct renderer_base_ops *ops)
{
	self->ops = ops;
	self->parent = parent;
	self->x = x;
	self->y = y;
	self->visible = 1;
	b6_list_initialize(&self->bases);
	b6_list_initialize(&self->tiles);
	if (parent)
		b6_list_add_last(&parent->bases, &self->dref);
	self->name = name;
}

static inline void move_renderer_base(struct renderer_base *self,
				      double x, double y)
{
	self->x = x;
	self->y = y;
}

static inline void show_renderer_base(struct renderer_base *self)
{
	self->visible = 1;
}

static inline void hide_renderer_base(struct renderer_base *self)
{
	self->visible = 0;
}

struct renderer_tile {
	struct b6_dref dref;
	const struct renderer_tile_ops *ops;
	struct renderer *renderer;
	struct renderer_base *parent;
	struct renderer_texture *texture;
	double x, y, w, h;
	void *userdata;
};

struct renderer_tile_ops {
	void (*dtor)(struct renderer_tile*);
};

static inline void __setup_renderer_tile(struct renderer_tile *self,
					 struct renderer_base *parent,
					 double x, double y, double w, double h,
					 struct renderer_texture *texture,
					 const struct renderer_tile_ops *ops)
{
	self->ops = ops;
	self->texture = texture;
	self->parent = parent;
	self->x = x;
	self->y = y;
	self->w = w;
	self->h = h;
	b6_list_add_last(&parent->tiles, &self->dref);
}

struct renderer_texture {
	const struct renderer_texture_ops *ops;
	struct renderer *renderer;
	void *userdata;
};

struct renderer_texture_ops {
	void (*update)(struct renderer_texture*, const struct rgba*);
	void (*dtor)(struct renderer_texture*);
};

struct renderer_observer {
	struct b6_dref dref;
	const struct renderer_observer_ops *ops;
	const char *debug_name;
};

struct renderer_observer_ops {
	void (*on_render)(struct renderer_observer*);
};

struct renderer {
	struct b6_entry entry;
	const struct renderer_ops *ops;
	struct b6_list observers;
	unsigned long int ntextures;
	unsigned long int ntiles;
	unsigned long int nbases;
	unsigned long int max_textures;
	unsigned long int max_tiles;
	unsigned long int max_bases;
	unsigned short int internal_width;
	unsigned short int internal_height;
	unsigned short int external_width;
	unsigned short int external_height;
};

static inline void __setup_renderer(struct renderer *self,
				    const struct renderer_ops *ops)
{
	self->ops = ops;
	b6_list_initialize(&self->observers);
}

struct renderer_ops {
	void (*start)(struct renderer*);
	void (*stop)(struct renderer*);
	void (*resize)(struct renderer*);
	void (*render)(struct renderer*);
	void (*dim)(struct renderer*, float);
	struct renderer_base *(*get_root)(struct renderer*);
	struct renderer_base *(*new_base)(struct renderer *renderer,
					  struct renderer_base *parent,
					  const char *name, double x, double y);
	struct renderer_tile *(*new_tile)(struct renderer *renderer,
					  struct renderer_base *parent,
					  double x, double y,
					  double w, double h,
					  struct renderer_texture *texture);
	struct renderer_texture *(*new_texture)(struct renderer*,
						const struct rgba*);
};

static inline void resize_renderer(struct renderer *self,
				   unsigned short int width,
				   unsigned short int height)
{
	self->external_width = width;
	self->external_height = height;
	if (self->ops->resize)
		self->ops->resize(self);
}

static inline void start_renderer(struct renderer *self,
				  unsigned short int width,
				  unsigned short int height)
{
	b6_assert(width);
	b6_assert(height);
	self->ntextures = self->max_textures = 0;
	self->ntiles = self->max_tiles = 0;
	self->nbases = self->max_bases = 0;
	self->internal_width = width;
	self->internal_height = height;
	if (self->ops->start)
		self->ops->start(self);
	if (self->ops->resize)
		self->ops->resize(self);
}

static inline void stop_renderer(struct renderer *self)
{
	if (self->ntextures)
		log_w("texture leak=%u", self->ntextures);
	if (self->ntiles)
		log_w("tile leak=%u", self->ntiles);
	if (self->nbases)
		log_w("base leak=%u", self->nbases);
	log_d("max textures=%u", self->max_textures);
	log_d("max tiles=%u", self->max_tiles);
	log_d("max bases=%u", self->max_bases);
	if (self->ops->stop)
		self->ops->stop(self);
}

static inline struct renderer_base *get_renderer_base(struct renderer *self)
{
	return self->ops->get_root(self);
}

static inline struct renderer_base *create_renderer_base(
	struct renderer *self, struct renderer_base *parent, const char *name,
	double x, double y)
{
	struct renderer_base *base =
		self->ops->new_base(self, parent, name, x, y);
	if (!base) {
		log_e("could not allocate base %s", name);
		return NULL;
	}
	base->renderer = self;
	self->nbases += 1;
	if (self->nbases > self->max_bases)
		self->max_bases = self->nbases;
	return base;
}

static inline struct renderer_base *create_renderer_base_or_die(
	struct renderer *self, struct renderer_base *parent, const char *name,
	double x, double y)
{
	struct renderer_base *base =
		create_renderer_base(self, parent, name, x, y);
	if (!base)
		log_p("giving up");
	return base;
}

extern void destroy_renderer_base_tiles(struct renderer_base *self);
extern void destroy_renderer_base_children(struct renderer_base *self);

static inline void destroy_renderer_base(struct renderer_base *self)
{
	if (!self)
		return;
	if (b6_unlikely(!self->renderer->nbases))
		log_p("double free");
	self->renderer->nbases -= 1;
	destroy_renderer_base_tiles(self);
	destroy_renderer_base_children(self);
	b6_list_del(&self->dref);
	if (self->ops->dtor)
		self->ops->dtor(self);
}

static inline struct renderer_tile *create_renderer_tile(
	struct renderer *self, struct renderer_base *parent, double x, double y,
	double w, double h, struct renderer_texture *texture)
{
	struct renderer_tile *tile =
		self->ops->new_tile(self, parent, x, y, w, h, texture);
	if (!tile) {
		log_e("could not allocate tile for base %s", parent->name);
		return NULL;
	}
	tile->renderer = self;
	self->ntiles += 1;
	if (self->ntiles > self->max_tiles)
		self->max_tiles = self->ntiles;
	return tile;
}

static inline struct renderer_tile *create_renderer_tile_or_die(
	struct renderer *self, struct renderer_base *parent, double x, double y,
	double w, double h, struct renderer_texture *texture)
{
	struct renderer_tile *tile =
		create_renderer_tile(self, parent, x, y, w, h, texture);
	if (!tile)
		log_p("giving up");
	return tile;
}

static inline void destroy_renderer_tile(struct renderer_tile *self)
{
	if (!self)
		return;
	if (b6_unlikely(!self->renderer->ntiles))
		log_p("double free");
	self->renderer->ntiles -= 1;
	b6_list_del(&self->dref);
	if (self->ops->dtor)
		self->ops->dtor(self);
}

static inline struct renderer_base *get_renderer_tile_base(
	const struct renderer_tile *self)
{
	return self->parent;
}

static inline void set_renderer_tile_texture(struct renderer_tile *self,
					     struct renderer_texture *texture)
{
	self->texture = texture;
}

static inline struct renderer_texture *get_renderer_tile_texture(
	const struct renderer_tile *self) 
{
	return self->texture;
}

static inline struct renderer_texture *create_renderer_texture(
	struct renderer *self, const struct rgba *rgba)
{
	struct renderer_texture *texture = self->ops->new_texture(self, rgba);
	if (!texture) {
		log_e("could not allocate texture");
		return NULL;
	}
	texture->renderer = self;
	self->ntextures += 1;
	if (self->ntextures > self->max_textures)
		self->max_textures = self->ntextures;
	return texture;
}

static inline struct renderer_texture *create_renderer_texture_or_die(
	struct renderer *self, const struct rgba *rgba)
{
	struct renderer_texture *texture = create_renderer_texture(self, rgba);
	if (!texture)
		log_p("giving up");
	return texture;
}

static inline void update_renderer_texture(struct renderer_texture *self,
					   const struct rgba *rgba)
{
	self->ops->update(self, rgba);
}

static inline void destroy_renderer_texture(struct renderer_texture *self)
{
	if (!self)
		return;
	if (b6_unlikely(!self->renderer->ntextures))
		log_p("double free");
	self->renderer->ntextures -= 1;
	return self->ops->dtor(self);
}

static inline void dim_renderer(struct renderer *self, float value)
{
	if (self->ops->dim)
		self->ops->dim(self, value);
}

extern void show_renderer(struct renderer*);

static inline struct renderer_observer *setup_renderer_observer(
	struct renderer_observer *self,
	const char *debug_name,
	const struct renderer_observer_ops *ops)
{
	self->ops = ops;
	self->debug_name = debug_name;
	b6_reset_observer(&self->dref);
	return self;
}

static inline void add_renderer_observer(struct renderer *w,
					 struct renderer_observer *o)
{
	if (b6_unlikely(b6_observer_is_attached(&o->dref)))
		log_w("observer %s is already registered", o->debug_name);
	else
		b6_attach_observer(&w->observers, &o->dref);
}

static inline void del_renderer_observer(struct renderer_observer *o)
{
	if (b6_unlikely(!b6_observer_is_attached(&o->dref)))
		log_w("observer %s is not registered", o->debug_name);
	else
		b6_detach_observer(&o->dref);
}

extern struct b6_registry __base_registry;

static inline int register_renderer(struct renderer *self, const char *name)
{
	return b6_register(&__base_registry, &self->entry, name);
}

static inline void unregister_renderer(struct renderer *self)
{
	b6_unregister(&__base_registry, &self->entry);
}

static inline struct renderer *lookup_renderer(const char *name)
{
	struct b6_entry *entry = b6_lookup_registry(&__base_registry, name);
	return entry ? b6_cast_of(entry, struct renderer, entry) : NULL;
}

#endif /*RENDERER_H*/
