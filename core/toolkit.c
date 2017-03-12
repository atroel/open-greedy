/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014-2017 Arnaud TROEL
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

#include "toolkit.h"

#include <b6/utf8.h>
#include <b6/utils.h>

#include "renderer.h"
#include "data.h"

void initialize_fixed_font(struct fixed_font *self, const struct rgba *rgba,
			   unsigned short int *x, unsigned short int *y,
			   unsigned short int w, unsigned short int h)
{
	int i;
	for (i = 0; i < b6_card_of(self->rgba); i += 1) {
		initialize_rgba(&self->rgba[i], w, h);
		copy_rgba(rgba, x[i], y[i], w, h, &self->rgba[i], 0, 0);
	}
}

void finalize_fixed_font(struct fixed_font *self)
{
	int i;
	for (i = 0; i < b6_card_of(self->rgba); i += 1)
		finalize_rgba(&self->rgba[i]);
}

static unsigned short int get_fixed_font_text_width(const struct fixed_font *f,
						    const struct b6_utf8 *utf8)
{
	unsigned short int w = 0;
	struct b6_utf8_iterator iter;
	b6_setup_utf8_iterator(&iter, utf8);
	while (b6_utf8_iterator_has_next(&iter)) {
		const int i = b6_utf8_iterator_get_next(&iter) - ' ';
		if (i < 0 || i >= b6_card_of(f->rgba))
			continue;
		w += f->rgba[i].w;
	}
	return w;
}

void render_fixed_font(const struct fixed_font *self,
		       const struct b6_utf8 *utf8, struct rgba *rgba,
		       unsigned short int x, unsigned short int y)
{
	struct b6_utf8_iterator iter;
	b6_setup_utf8_iterator(&iter, utf8);
	while (b6_utf8_iterator_has_next(&iter)) {
		int i = b6_utf8_iterator_get_next(&iter) - ' ';
		const struct rgba *from;
		if (i < 0 || i >= b6_card_of(self->rgba))
			continue;
		from = &self->rgba[i];
		copy_rgba(from, 0, 0, from->w, from->h, rgba, x, y);
		x += from->w;
	}
}

struct renderer_texture *make_texture(struct renderer *renderer,
				      const char *skin_id, const char *data_id)
{
	struct renderer_texture *texture = NULL;
	struct data_entry *entry;
	struct image_data *data;
	struct rgba rgba;
	if (get_image_data(skin_id, data_id, NULL, &entry, &data))
		goto bail_out;
	if (!initialize_rgba(&rgba, data->w, data->h)) {
		copy_rgba(data->rgba, *data->x, *data->y, data->w, data->h,
			  &rgba, 0, 0);
		texture = create_renderer_texture(renderer, &rgba);
		finalize_rgba(&rgba);
	} else
		log_w(_s("out of memory"));
	put_image_data(entry, data);
bail_out:
	return texture;
}

int make_font(struct fixed_font *font, const char *skin_id, const char *data_id)
{
	struct data_entry *entry;
	struct image_data *data;
	if (get_image_data(skin_id, data_id, NULL, &entry, &data))
		return -1;
	initialize_fixed_font(font, data->rgba, data->x + ' ', data->y + ' ',
			      data->w, data->h);
	put_image_data(entry, data);
	return 0;
}

void initialize_toolkit_label(struct toolkit_label *self,
			      struct renderer *renderer,
			      const struct fixed_font *font,
			      unsigned short int u, unsigned short int v,
			      struct renderer_base *base,
			      float x, float y, float w, float h)
{
	self->renderer = renderer;
	self->font = font;
	initialize_rgba(&self->rgba, u, v);
	clear_rgba(&self->rgba, 0x00000000);
	initialize_toolkit_image(&self->image[1], renderer, base,
				 x + 3, y + 3, w, h, NULL);
	initialize_toolkit_image(&self->image[0], renderer, base, x, y, w, h,
				 create_renderer_texture_or_die(renderer,
								&self->rgba));
}

void finalize_toolkit_label(struct toolkit_label *self)
{
	destroy_renderer_texture(self->image[0].texture);
	if (self->image[1].texture)
		destroy_renderer_texture(self->image[1].texture);
	finalize_toolkit_image(&self->image[0]);
	finalize_toolkit_image(&self->image[1]);
	finalize_rgba(&self->rgba);
}

void enable_toolkit_label_shadow(struct toolkit_label *self)
{
	if (self->image[1].texture)
		return;
	make_shadow_rgba(&self->rgba);
	self->image[1].texture = create_renderer_texture_or_die(self->renderer,
								&self->rgba);
	if (self->image[0].tile->texture)
		show_toolkit_image(&self->image[1]);
}

void disable_toolkit_label_shadow(struct toolkit_label *self)
{
	struct renderer_texture *texture = self->image[1].texture;
	if (texture) {
		self->image[1].texture = NULL;
		destroy_renderer_texture(texture);
	}
}

int set_toolkit_label(struct toolkit_label *self, const struct b6_utf8 *utf8)
{
	int x, y;
	if (!self->font)
		return 0;
	x = self->rgba.w - get_fixed_font_text_width(self->font, utf8);
	y = self->rgba.h - get_fixed_font_height(self->font);
	if (x < 0 || y < 0)
		return -1;
	render_fixed_font(self->font, utf8, &self->rgba, x / 2, y / 2);
	update_renderer_texture(self->image[0].texture, &self->rgba);
	if (self->image[1].texture) {
		make_shadow_rgba(&self->rgba);
		update_renderer_texture(self->image[1].texture, &self->rgba);
	}
	return 0;
}

void initialize_toolkit_image(struct toolkit_image *self,
			      struct renderer *renderer, struct renderer_base *base,
			      float x, float y, float w, float h,
			      struct renderer_texture *texture)
{
	self->tile = create_renderer_tile_or_die(renderer, base, x, y, w, h,
						 texture);
	self->texture = texture;
}

void finalize_toolkit_image(struct toolkit_image *self)
{
	destroy_renderer_tile(self->tile);
}

void show_toolkit_image(struct toolkit_image *self)
{
	set_renderer_tile_texture(self->tile, self->texture);
}

void hide_toolkit_image(struct toolkit_image *self)
{
	set_renderer_tile_texture(self->tile, NULL);
}
