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

#ifndef TOOLKIT_H
#define TOOLKIT_H

#include "rgba.h"
#include "renderer.h"

struct fixed_font {
	struct rgba rgba[96];
};

extern void initialize_fixed_font(struct fixed_font *self,
				  const struct rgba *rgba,
				  unsigned short int *x, unsigned short int *y,
				  unsigned short int w, unsigned short int h);

extern void finalize_fixed_font(struct fixed_font *self);

extern void render_fixed_font(const struct fixed_font *self, const char *s,
			      struct rgba *rgba,
			      unsigned short int x, unsigned short int y);

extern void render_fixed_font_utf8(const struct fixed_font *self,
				   const void *s, unsigned int n,
				   struct rgba *rgba,
				   unsigned short int x, unsigned short int y);

static inline unsigned short int get_fixed_font_width(
	const struct fixed_font *self)
{
	return self->rgba[0].w;
}

static inline unsigned short int get_fixed_font_height(
	const struct fixed_font *self)
{
	return self->rgba[0].h;
}

extern struct renderer_texture *make_texture(struct renderer *renderer,
					     const char *skin_id,
					     const char *data_id);

extern int make_font(struct fixed_font *font, const char *skin_id,
		     const char *data_id);

struct toolkit_image {
	struct renderer_tile *tile;
	struct renderer_texture *texture;
};

extern void initialize_toolkit_image(struct toolkit_image *self,
				     struct renderer *renderer,
				     struct renderer_base *base,
				     float x, float y, float w, float h,
				     struct renderer_texture *texture);

extern void finalize_toolkit_image(struct toolkit_image *self);

extern void show_toolkit_image(struct toolkit_image *self);

extern void hide_toolkit_image(struct toolkit_image *self);

struct toolkit_label {
	struct renderer *renderer;
	const struct fixed_font *font;
	struct rgba rgba;
	struct toolkit_image image[2];
};

extern void initialize_toolkit_label(struct toolkit_label *self,
				     struct renderer *renderer,
				     const struct fixed_font *font,
				     unsigned short int u, unsigned short int v,
				     struct renderer_base *base,
				     float x, float y, float w, float h);

extern void finalize_toolkit_label(struct toolkit_label *self);

extern void enable_toolkit_label_shadow(struct toolkit_label *self);

extern void disable_toolkit_label_shadow(struct toolkit_label *self);

extern int set_toolkit_label(struct toolkit_label *self, const char *text);

extern int set_toolkit_label_utf8(struct toolkit_label *self, const void *utf8,
				  unsigned int len);

static inline void show_toolkit_label(struct toolkit_label *self)
{
	show_toolkit_image(&self->image[0]);
	show_toolkit_image(&self->image[1]);
}

static inline void hide_toolkit_label(struct toolkit_label *self)
{
	hide_toolkit_image(&self->image[0]);
	hide_toolkit_image(&self->image[1]);
}

#endif /* TOOLKIT_H */
