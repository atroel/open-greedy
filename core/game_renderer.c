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

#include "game_renderer.h"

#include <b6/json.h>
#include <b6/utf8.h>

#include "lib/std.h"

#include "game.h"
#include "game_phase.h"
#include "data.h"
#include "renderer.h"

#define for_each_gum(curr, head) \
	for (curr = head; curr; curr = curr->tile->userdata)

#define for_each_gum_safe(curr, next, head) \
	for (curr = head; curr && ((next = curr->tile->userdata) || 1); \
	     curr = next)

static int create_cartoon(struct cartoon *cartoon, struct renderer *renderer,
			  const char *skin_id, const char *data_id, int loop)
{

	struct data_entry *entry;
	struct image_data *data;
	struct renderer_texture *curr = NULL;
	struct rgba rgba;
	unsigned int i;
	int retval = 0;
	cartoon->index = 0;
	cartoon->texture = NULL;
	if (get_image_data(skin_id, data_id, NULL, &entry, &data)) {
		retval = -1;
		goto fail_image;
	}
	cartoon->length = data->length;
	cartoon->period = data->period;
	cartoon->loop = loop;
	cartoon->time = 0;
	if (initialize_rgba(&rgba, data->w, data->h)) {
		retval = -2;
		goto fail_rgba;
	}
	for (i = 0; i < data->length ; i += 1) {
		struct renderer_texture *next;
		copy_rgba(data->rgba, data->x[i], data->y[i], data->w, data->h,
			  &rgba, 0, 0);
		next = create_renderer_texture(renderer, &rgba);
		if (curr)
			curr->userdata = next;
		else
			cartoon->texture = next;
		if (!(curr = next)) {
			retval = -3;
			break;
		}
	}
	if (!curr) {
		retval = -3;
		curr = cartoon->texture;
		while (curr) {
			struct renderer_texture *next = curr->userdata;
			destroy_renderer_texture(curr);
			curr = next;
		}
		goto fail_texture;
	}
	curr->userdata = cartoon->texture;
fail_texture:
	finalize_rgba(&rgba);
fail_rgba:
	put_image_data(entry, data);
fail_image:
	return retval;
}

static void destroy_cartoon(struct cartoon *cartoon)
{
	struct renderer_texture *curr;
	if ((curr = cartoon->texture))
		do {
			struct renderer_texture *next = curr->userdata;
			destroy_renderer_texture(curr);
			curr = next;
		} while (curr != cartoon->texture);
}

static void reset_cartoon(struct cartoon *self, unsigned long long int now)
{
	self->time = now;
}

static void copy_cartoon(const struct cartoon *from, struct cartoon *to)
{
	to->texture = from->texture;
	to->index = from->index;
	to->length = from->length;
	to->period = from->period;
	to->loop = from->loop;
	to->time = from->time;
}

static unsigned long long int get_cartoon_duration(const struct cartoon *self)
{
	return self->period * self->length;
}

static struct renderer_texture *get_cartoon_image(const struct cartoon *self,
						  unsigned long long int now)
{
	unsigned long int d, n;
	if (!self->texture)
		goto bail_out;
	n = (now - self->time) / self->period;
	if (n >= self->length)
		n = self->loop ? n % self->length : self->length - 1;
	if (n < self->index)
		d = n + self->length - self->index;
	else
		d = n - self->index;
	((struct cartoon*)self)->index = n;
	while (d--)
		((struct cartoon*)self)->texture = self->texture->userdata;
bail_out:
	return self->texture;
}

static struct game_renderer* to_game_renderer(struct game_observer *observer)
{
	return b6_cast_of(observer, struct game_renderer, game_observer);
}

static void drain_game_renderer_infos(struct game_renderer *self)
{
	struct state *state;
	while ((state = current_state(&self->info_queue)))
	     __dequeue_state(state, &self->info_queue);
}

static struct game_renderer_info *to_game_renderer_info(struct state *up)
{
	return b6_cast_of(up, struct game_renderer_info, up);
}

static void unpublish_game_renderer_info(struct game_renderer_info *self)
{
	dequeue_state(&self->up, &self->game_renderer->info_queue);
}

static void publish_game_renderer_info(struct game_renderer_info *self)
{
	enqueue_state(&self->up, &self->game_renderer->info_queue);
}

static void game_renderer_info_on_enqueue(struct state *up)
{
	struct game_renderer_info *self = to_game_renderer_info(up);
	set_renderer_tile_texture(self->game_renderer->info_tile,
				  self->texture);
	set_renderer_tile_texture(self->game_renderer->info_icon_tile,
				  self->icon_texture);
}

static void game_renderer_info_on_enqueue_volatile(struct state *up)
{
	struct game_renderer_info *self = to_game_renderer_info(up);
	self->expire = self->game_renderer->time + 3000000ULL;
	game_renderer_info_on_enqueue(up);
}

static void game_renderer_info_on_dequeue(struct state *up)
{
	struct game_renderer_info *self = to_game_renderer_info(up);
	if (current_state(&self->game_renderer->info_queue) != up)
		return;
	set_renderer_tile_texture(self->game_renderer->info_tile, NULL);
	set_renderer_tile_texture(self->game_renderer->info_icon_tile, NULL);
}

static void game_renderer_info_promote_on_demote(struct state *up)
{
	struct game_renderer_info *self = to_game_renderer_info(up);
	struct state *state;
	__dequeue_state(up, &self->game_renderer->info_queue);
	state = current_state(&self->game_renderer->info_queue);
	if (state->ops->on_demote == game_renderer_info_promote_on_demote)
		return;
	__enqueue_state(up, &self->game_renderer->info_queue);
}

static void game_renderer_info_dequeue_on_demote(struct state *up)
{
	struct game_renderer_info *self = to_game_renderer_info(up);
	__dequeue_state(up, &self->game_renderer->info_queue);
}

static const struct state_ops standard_info_ops = {
	.on_enqueue = game_renderer_info_on_enqueue,
	.on_dequeue = game_renderer_info_on_dequeue,
	.on_promote = game_renderer_info_on_enqueue,
};

static const struct state_ops sticky_info_ops = {
	.on_enqueue = game_renderer_info_on_enqueue,
	.on_dequeue = game_renderer_info_on_dequeue,
	.on_promote = game_renderer_info_on_enqueue,
	.on_demote = game_renderer_info_promote_on_demote,
};

static const struct state_ops volatile_info_ops = {
	.on_enqueue = game_renderer_info_on_enqueue_volatile,
	.on_dequeue = game_renderer_info_on_dequeue,
	.on_demote = game_renderer_info_dequeue_on_demote,
};

static int initialize_game_renderer_info(struct game_renderer_info *self,
					 struct game_renderer *game_renderer,
					 struct rgba *rgba,
					 const struct b6_json_object *lang,
					 const struct b6_utf8 *key,
					 const char *data_id,
					 const struct state_ops *ops)
{
	unsigned short int font_w, font_h;
	struct b6_json_string *text = b6_json_get_object_as(lang, key, string);
	self->texture = self->icon_texture = NULL;
	font_w = get_fixed_font_width(&game_renderer->font);
	font_h = get_fixed_font_height(&game_renderer->font);
	if (font_h + 1 > rgba->h)
		return 0;
	clear_rgba(rgba, 0xff000000);
	if (text) {
		const struct b6_utf8 *utf8 = b6_json_get_string(text);
		int x = rgba->w - (utf8->nchars) * font_w;
		if (x < 0)
			return 0;
		render_fixed_font(&game_renderer->font, utf8, rgba, x / 2, 1);
	}
	self->texture = create_renderer_texture(game_renderer->renderer, rgba);
	self->icon_texture = make_texture(game_renderer->renderer,
					  game_renderer->skin_id, data_id);
	self->game_renderer = game_renderer;
	reset_state(&self->up, ops);
	return 0;
}

static void finalize_game_renderer_info(struct game_renderer_info *self)
{
	if (self->icon_texture)
		destroy_renderer_texture(self->icon_texture);
	if (self->texture)
		destroy_renderer_texture(self->texture);
}

static void game_renderer_count_on_render(struct renderer_observer *obs)
{
	struct game_renderer_count *self =
		b6_cast_of(obs, struct game_renderer_count, observer);
	if (!self->dirty)
		return;
	clear_rgba(&self->rgba, 0xff000000);
	self->dirty = 0;
	if (!self->icon.p || self->value > 3) {
		struct b6_utf8 utf8;
		char s[] = "00";
		s[0] += (self->value / 10) % 10;
		s[1] += self->value % 10;
		render_fixed_font(&self->game_renderer->font,
				  b6_utf8_from_ascii(&utf8, s),
				  &self->rgba, 0, 1);
		if (self->icon.p)
			copy_rgba(&self->icon, 0, 0, 16, 16,
				  &self->rgba, 42, 1);
	} else {
		unsigned int count;
		for (count = self->value; count; count -= 1)
			copy_rgba(&self->icon, 0, 0, 16, 16,
				  &self->rgba, 54 - count * 16, 1);
	}
	update_renderer_texture(self->tile->texture, &self->rgba);
}

static void initialize_game_renderer_count(struct game_renderer_count *self,
					   struct game_renderer *game_renderer,
					   struct renderer_base *base,
					   float x, float y,
					   const char *data_id)
{
	static const struct renderer_observer_ops ops = {
		.on_render = game_renderer_count_on_render,
	};
	struct renderer_texture *texture;
	struct data_entry *entry;
	struct image_data *data;
	self->game_renderer = game_renderer;
	self->value = 0;
	self->dirty = 1;
	if (initialize_rgba(&self->rgba, 58, 18))
		return;
	clear_rgba(&self->rgba, 0xff000000);
	if (!(self->tile = create_renderer_tile(game_renderer->renderer, base,
						x, y, 58, 18, NULL)))
		return;
	self->icon.p = NULL;
	texture = create_renderer_texture(game_renderer->renderer,
					  &self->rgba);
	if (!texture)
		return;
	set_renderer_tile_texture(self->tile, texture);
	if (get_image_data(game_renderer->skin_id, data_id, NULL,
			   &entry, &data))
		return;
	if (!initialize_rgba(&self->icon, data->w, data->h)) {
		copy_rgba(data->rgba, *data->x, *data->y, data->w, data->h,
			  &self->icon, 0, 0);
		setup_renderer_observer(&self->observer, "count", &ops);
		add_renderer_observer(game_renderer->renderer, &self->observer);
	}
	put_image_data(entry, data);
}

static void finalize_game_renderer_count(struct game_renderer_count *self)
{
	finalize_rgba(&self->rgba);
	if (!self->tile)
		return;
	if (self->icon.p)
		del_renderer_observer(&self->observer);
	finalize_rgba(&self->icon);
	destroy_renderer_texture(self->tile->texture);
	destroy_renderer_tile(self->tile);
}

static void set_game_renderer_count(struct game_renderer_count *self,
				    unsigned int count)
{
	self->dirty = 1;
	self->value = count;
}

static void game_renderer_gauge_on_render(struct renderer_observer *obs)
{
	struct game_renderer_gauge *self =
		b6_cast_of(obs, struct game_renderer_gauge, observer);
	if (self->rendered_width == self->notified_width)
		return;
	if (self->rendered_width > self->notified_width)
		self->rendered_width -= 1;
	else
		self->rendered_width += 1;
	copy_rgba(&self->skin, 0, self->rgba.h, self->rgba.w, self->rgba.h,
		  &self->rgba, 0, 0);
	copy_rgba(&self->skin, 0, 0, self->rendered_width, self->rgba.h,
		  &self->rgba, 0, 0);
	update_renderer_texture(self->tile->texture, &self->rgba);
}

static void initialize_game_renderer_gauge(struct game_renderer_gauge *self,
					   struct game_renderer *game_renderer,
					   struct renderer_base *base,
					   float x, float y, float w, float h,
					   const char *data_id)
{
	static const struct renderer_observer_ops ops = {
		.on_render = game_renderer_gauge_on_render,
	};
	struct data_entry *entry;
	struct image_data *data;
	struct renderer_texture *texture;
	if (!(self->tile = create_renderer_tile(game_renderer->renderer, base,
						x, y, w, h, NULL)))
		return;
	self->skin.p = self->rgba.p = NULL;
	if (get_image_data(game_renderer->skin_id, data_id, NULL,
			   &entry, &data))
		return;
	self->game_renderer = game_renderer;
	self->value = 1.0;
	self->notified_width = data->w;
	self->rendered_width = self->notified_width - 1;
	if (initialize_rgba(&self->rgba, data->w, data->h))
		goto bail_out;
	if (initialize_rgba(&self->skin, data->w, data->h * 2))
		goto bail_out;
	copy_rgba(data->rgba, data->x[0], data->y[0], data->w, data->h,
		  &self->skin, 0, 0);
	copy_rgba(data->rgba, data->x[1], data->y[1], data->w, data->h,
		  &self->skin, 0, data->h);
	texture = create_renderer_texture(game_renderer->renderer, &self->rgba);
	if (!texture)
		goto bail_out;
	set_renderer_tile_texture(self->tile, texture);
	setup_renderer_observer(&self->observer, "gauge", &ops);
	add_renderer_observer(game_renderer->renderer, &self->observer);
bail_out:
	put_image_data(entry, data);
}

static void finalize_game_renderer_gauge(struct game_renderer_gauge *self)
{
	if (!self->tile)
		return;
	if (self->rgba.p && self->skin.p)
		del_renderer_observer(&self->observer);
	finalize_rgba(&self->rgba);
	finalize_rgba(&self->skin);
	destroy_renderer_texture(self->tile->texture);
	destroy_renderer_tile(self->tile);
}

static void set_game_renderer_gauge(struct game_renderer_gauge *self,
				    float value)
{
	self->value = value;
	self->notified_width = self->value * self->rgba.w;
}

static void game_renderer_sprite_on_render(struct renderer_observer *obs)
{
	struct game_renderer_sprite *self =
		b6_cast_of(obs, struct game_renderer_sprite, observer);
	set_renderer_tile_texture(
		self->tile, get_cartoon_image(
			self->cartoon, self->game_renderer->time));
}

static void set_game_renderer_sprite_cartoon(struct game_renderer_sprite *self,
					     const struct cartoon *cartoon)
{
	struct renderer *renderer = self->game_renderer->renderer;
	if (!self->tile)
		return;
	if (self->cartoon != NULL)
		del_renderer_observer(&self->observer);
	self->cartoon = cartoon;
	if (cartoon != NULL) {
		add_renderer_observer(renderer, &self->observer);
	} else
		set_renderer_tile_texture(self->tile, NULL);
}

static void move_game_renderer_sprite(struct game_renderer_sprite *self,
				      double x, double y)
{
	move_renderer_base(self->tile->parent, x * 16, y * 16);
}

static int initialize_game_renderer_sprite(struct game_renderer_sprite *self,
					   struct game_renderer *game_renderer,
					   struct renderer_base *base,
					   float x, float y, float w, float h)
{
	static const struct renderer_observer_ops ops = {
		.on_render = game_renderer_sprite_on_render,
	};
	setup_renderer_observer(&self->observer, "game_sprite", &ops);
	self->game_renderer = game_renderer;
	self->cartoon = NULL;
	self->tile = create_renderer_tile(game_renderer->renderer, base,
					  x, y, w, h, NULL);
	return 0;
}

static void finalize_game_renderer_sprite(struct game_renderer_sprite *self)
{
	set_game_renderer_sprite_cartoon(self, NULL);
	destroy_renderer_tile(self->tile);
}

static int initialize_game_renderer_casino(struct game_renderer_casino *self,
					   struct game_renderer *game_renderer,
					   const struct b6_json_object *lang,
					   struct rgba *scratch)
{
	struct data_entry *entry;
	struct image_data *data;
	int i;
	if ((initialize_rgba(&self->rgba, scratch->w, scratch->h)))
		return -1;
	clear_rgba(&self->rgba, 0xff000000);
	initialize_game_renderer_info(&self->game_renderer_info, game_renderer,
				      &self->rgba, lang, B6_UTF8("casino"),
				      GAME_INFO_CASINO_DATA_ID,
				      &sticky_info_ops);
	self->roll_image.p = NULL;
	if (get_image_data(game_renderer->skin_id, GAME_CASINO_DATA_ID, NULL,
			   &entry, &data))
		return 0;
	self->roll_width = data->w;
	self->roll_count = data->length;
	self->roll_index[0] = self->roll_index[1] = self->roll_index[2] = -1;
	if (!initialize_rgba(&self->roll_image, data->w * data->length,
			     data->h))
		for (i = 0; i < data->length; i += 1)
			copy_rgba(data->rgba, data->x[i], data->y[i],
				  data->w, data->h,
				  &self->roll_image, i * data->w, 0);
	put_image_data(entry, data);
	return 0;
}

static void launch_game_renderer_casino(struct game_renderer_casino *self)
{
	struct game_renderer_info *info = &self->game_renderer_info;
	info->expire = 0ULL;
	publish_game_renderer_info(info);
}

static void finish_game_renderer_casino(struct game_renderer_casino *self)
{
	struct game_renderer_info *info = &self->game_renderer_info;
	info->expire =
		b6_get_clock_time(info->game_renderer->clock) + 1000000ULL;
}

static void update_game_renderer_casino(struct game_renderer_casino *self,
					struct game_casino *casino)
{
	struct game_renderer_info *info = &self->game_renderer_info;
	unsigned short int x = self->rgba.w - 16;
	int i, dirty = 0;
	if (!self->roll_image.p)
		return;
	for (i = b6_card_of(casino->value); i--;) {
		unsigned int n = casino->value[i] * self->roll_count / 4;
		x -= self->roll_width + 2;
		if (n == self->roll_index[i])
			continue;
		dirty = 1;
		self->roll_index[i] = n;
		copy_rgba(&self->roll_image, n * self->roll_width, 0,
			  self->roll_width, self->roll_image.h,
			  &self->rgba, x, 0);
	}
	if (dirty)
		update_renderer_texture(info->texture, &self->rgba);
}

static void finalize_game_renderer_casino(struct game_renderer_casino *self)
{
	finalize_game_renderer_info(&self->game_renderer_info);
	finalize_rgba(&self->rgba);
	finalize_rgba(&self->roll_image);
}

static void update_game_renderer_jewels(struct game_renderer_jewels *self,
					unsigned long int jewels_bitmask)
{
	int i;
	if (!self->game_renderer)
		return;
	if (jewels_bitmask == (1 << b6_card_of(self->tiles)) - 1) {
		if (linear_is_stopped(&self->linear))
			add_renderer_observer(self->game_renderer->renderer,
					      &self->renderer_observer);
		reset_linear(&self->linear, 0, 2 * b6_card_of(self->tiles),
			     1e-5);
	} else if (!linear_is_stopped(&self->linear)) {
		reset_linear(&self->linear, 0, 0, 0);
		del_renderer_observer(&self->renderer_observer);
	}
	for (i = 0; i < b6_card_of(self->tiles); i += 1, jewels_bitmask >>= 1)
		if (jewels_bitmask & 1)
			set_renderer_tile_texture(self->tiles[i],
						  self->textures[i]);
		else
			set_renderer_tile_texture(self->tiles[i], NULL);
}

static void game_renderer_jewels_on_render(struct renderer_observer *obs)
{
	struct game_renderer_jewels *self =
		b6_cast_of(obs, struct game_renderer_jewels, renderer_observer);
	unsigned long int i, j;
	if (linear_is_stopped(&self->linear)) {
		del_renderer_observer(obs);
		return;
	}
	j = update_linear(&self->linear);
	while (j >= b6_card_of(self->tiles))
		j -= b6_card_of(self->tiles);
	for (i = 0; i < b6_card_of(self->tiles); i += 1) {
		set_renderer_tile_texture(self->tiles[i], self->textures[j]);
		j += 1;
		if (b6_unlikely(j == b6_card_of(self->tiles)))
			j = 0;
	}
}

static void finalize_game_renderer_jewels(struct game_renderer_jewels *self)
{
	int i;
	if (!self->game_renderer)
		return;
	if (!linear_is_stopped(&self->linear))
		del_renderer_observer(&self->renderer_observer);
	for (i = 0; i < b6_card_of(self->tiles); i += 1) {
		destroy_renderer_texture(self->textures[i]);
		destroy_renderer_tile(self->tiles[i]);
	}
}

static int initialize_game_renderer_jewel(struct renderer *renderer,
					  const char *skin_id,
					  const char *data_id,
					  short int x, short int y,
					  struct renderer_base *base,
					  struct renderer_tile **tile,
					  struct renderer_texture **texture)
{
	*texture = make_texture(renderer, skin_id, data_id);
	*tile = create_renderer_tile(renderer, base, x, y, 32, 32, NULL);
	return *texture && *tile ? 0 : -1;
}

static int initialize_game_renderer_jewels(struct game_renderer_jewels *self,
					   struct game_renderer *game_renderer,
					   struct renderer_base *base)
{
	static const struct renderer_observer_ops ops = {
		.on_render = game_renderer_jewels_on_render,
	};
	struct renderer *renderer = game_renderer->renderer;
	const char *skin_id = game_renderer->skin_id;
	int retval = 0;
	self->game_renderer = game_renderer;
	setup_renderer_observer(&self->renderer_observer, "game_jewels", &ops);
	setup_linear(&self->linear, game_renderer->clock, 0, 0, 0);
	retval |= initialize_game_renderer_jewel(
		renderer, skin_id, GAME_JEWEL_DATA_ID(1), 14, 20, base,
		&self->tiles[0], &self->textures[0]);
	retval |= initialize_game_renderer_jewel(
		renderer, skin_id, GAME_JEWEL_DATA_ID(2), 30, 6, base,
		&self->tiles[1], &self->textures[1]);
	retval |= initialize_game_renderer_jewel(
		renderer, skin_id, GAME_JEWEL_DATA_ID(3), 53, -4, base,
		&self->tiles[2], &self->textures[2]);
	retval |= initialize_game_renderer_jewel(
		renderer, skin_id, GAME_JEWEL_DATA_ID(4), 78, -8, base,
		&self->tiles[3], &self->textures[3]);
	retval |= initialize_game_renderer_jewel(
		renderer, skin_id, GAME_JEWEL_DATA_ID(5), 102, -4, base,
		&self->tiles[4], &self->textures[4]);
	retval |= initialize_game_renderer_jewel(
		renderer, skin_id, GAME_JEWEL_DATA_ID(6), 124, 6, base,
		&self->tiles[5], &self->textures[5]);
	retval |= initialize_game_renderer_jewel(
		renderer, skin_id, GAME_JEWEL_DATA_ID(7), 140, 20, base,
		&self->tiles[6], &self->textures[6]);
	if (retval) {
		log_w(_s("cannot allocate jewels"));
		self->game_renderer = NULL;
		finalize_game_renderer_jewels(self);
	}
	return retval;
}

static void game_renderer_popup_on_render(struct renderer_observer *obs)
{
	struct game_renderer_popup *self =
		b6_cast_of(obs, struct game_renderer_popup, renderer_observer);
	struct renderer_base *base = self->tile->parent;
	if (b6_likely(!linear_is_stopped(&self->linear))) {
		double x = base->x;
		double y = update_linear(&self->linear);
		move_renderer_base(base, x, y);
	} else {
		del_renderer_observer(obs);
		hide_renderer_base(base);
	}
}

static int initialize_game_renderer_popup(struct game_renderer_popup *self,
					  struct game_renderer *game_renderer,
					  struct renderer_base *parent,
					  const char *data_id)
{
	static const struct renderer_observer_ops ops = {
		.on_render = game_renderer_popup_on_render,
	};
	struct data_entry *entry;
	struct image_data *data;
	struct renderer_base *base;
	struct renderer_texture *texture;
	struct rgba rgba;
	self->renderer = NULL;
	if (get_image_data(game_renderer->skin_id, data_id, NULL,
			   &entry, &data))
		return 0;
	if (initialize_rgba(&rgba, data->w, data->h))
		goto bail_out;
	copy_rgba(data->rgba, data->x[0], data->y[0], data->w, data->h,
		  &rgba, 0, 0);
	texture = create_renderer_texture(game_renderer->renderer, &rgba);
	finalize_rgba(&rgba);
	if (!texture)
		goto bail_out;
	if (!(base = create_renderer_base(game_renderer->renderer, parent,
					  data_id, 0, 0))) {
		destroy_renderer_texture(texture);
		goto bail_out;
	}
	if (!(self->tile = create_renderer_tile(game_renderer->renderer, base,
						0, 0, data->w, data->h,
						texture))) {
		destroy_renderer_base(base);
		destroy_renderer_texture(texture);
		goto bail_out;
	}
	setup_linear(&self->linear, game_renderer->clock, 0, 0, 0);
	hide_renderer_base(base);
	setup_renderer_observer(&self->renderer_observer, "game_popup", &ops);
	self->renderer = game_renderer->renderer;
bail_out:
	put_image_data(entry, data);
	return 0;
}

static void finalize_game_renderer_popup(struct game_renderer_popup *self)
{
	if (!self->renderer)
		return;
	if (!linear_is_stopped(&self->linear))
		del_renderer_observer(&self->renderer_observer);
	destroy_renderer_texture(self->tile->texture);
	destroy_renderer_base(self->tile->parent);
}

static void trigger_game_renderer_popup(struct game_renderer_popup *self,
					double x, double y)
{
	struct renderer_base *base;
	if (!self->renderer)
		return;
	base = self->tile->parent;
	move_renderer_base(base, x, y);
	show_renderer_base(base);
	if (linear_is_stopped(&self->linear))
		add_renderer_observer(self->renderer, &self->renderer_observer);
	reset_linear(&self->linear, y, y - 64, 15e-5);
}

static void on_level_init(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	struct layout *layout = &self->game->layout;
	struct level *level = &self->game->level;
	struct items *items = level->items;
	int x, y;
	char s[] = "000";
	struct level_iterator iterator;
	struct data_entry *entry;
	struct image_data *data;
	struct b6_utf8 utf8;

	if (!(get_image_data(self->skin_id, GAME_LAYOUT_DATA_ID, layout, &entry,
			     &data))) {
		update_renderer_texture(
			get_renderer_tile_texture(self->playground),
			data->rgba);
		put_image_data(entry, data);
	} else
		log_e(_s("cannot update playground texture"));

	initialize_level_iterator(&iterator, level);
	self->pacgums = NULL;
	self->super_pacgums = NULL;
	while (level_iterator_has_next(&iterator)) {
		struct place *place = level_iterator_next(&iterator);
		struct item *item = place->item;
		struct toolkit_image *image;
		place_location(level, place, &x, &y);
		image = &self->gums[x][y];
		if (is_pacgum_item(items, item)) {
			initialize_toolkit_image(
				image, self->renderer,
				get_renderer_tile_base(self->playground),
				16 * x + 12, 16 * y + 12, 8, 8,
				self->pacgum_texture);
			image->tile->userdata = self->pacgums;
			self->pacgums = image;
		} else if (is_super_pacgum_item(items, item)) {
			initialize_toolkit_image(
				image, self->renderer,
				get_renderer_tile_base(self->playground),
				16 * x + 8, 16 * y + 8, 16, 16,
				self->super_pacgum_texture);
			image->tile->userdata = self->super_pacgums;
			self->super_pacgums = image;
		}
	}

	if (level->bonus_place) {
		place_location(level, level->bonus_place, &x, &y);
		move_game_renderer_sprite(&self->bonus, x, y);
	}

	b6_reset_event(&self->hold.event, NULL);
	self->rendered_score = self->notified_score - 1;

	s[0] += (self->game->n / 100) % 10;
	s[1] += (self->game->n / 10) % 10;
	s[2] += self->game->n % 10;
	set_toolkit_label(&self->level, b6_utf8_from_ascii(&utf8, s));
}

static void on_level_exit(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	struct toolkit_image *curr, *next;
	set_game_renderer_sprite_cartoon(&self->bonus, NULL);
	for_each_gum_safe(curr, next, self->super_pacgums)
		finalize_toolkit_image(curr);
	self->super_pacgums = NULL;
	for_each_gum_safe(curr, next, self->pacgums)
		finalize_toolkit_image(curr);
	self->pacgums = NULL;
}

static void on_level_enter(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	set_fade_io_target(&self->fade_io, 1);
	hold_game_for(self->game, &self->hold, 500000);
}

static void on_level_leave(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	set_fade_io_target(&self->fade_io, 0);
	hold_game_for(self->game, &self->hold, 500000);
	drain_game_renderer_infos(self);
}

static void on_level_start(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	self->pacman_direction = LEVEL_E;
	self->pacman_state = self->locked_state = self->afraid_state = -1;
	set_game_renderer_count(&self->shields, self->game->pacman.shields);
	set_game_renderer_count(&self->lifes, self->game->pacman.lifes);
}

static void on_level_touch(struct game_observer *game_observer,
			   struct place *place,
			   struct item *item)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	struct items *items = self->game->level.items;
	int x, y;
	place_location(&self->game->level, place, &x, &y);
	if (is_pacgum_item(items, item) || is_super_pacgum_item(items, item)) {
		hide_toolkit_image(&self->gums[x][y]);
		self->gums[x][y].texture = NULL;
	} else if (is_bonus_item(items, place->item)) {
		enum bonus_type contents = get_bonus_contents(place->item);
		set_game_renderer_sprite_cartoon(
			&self->bonus, &self->bonus_cartoons[contents]);
	} else if (is_bonus_item(items, item))
		set_game_renderer_sprite_cartoon(&self->bonus, NULL);
}

static void on_level_passed(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	reset_cartoon(&self->won, self->time);
	set_game_renderer_sprite_cartoon(&self->pacman, &self->won);
	self->pacman_state = 2;
	if (!b6_event_is_pending(&self->hold.event))
		hold_game_for(self->game, &self->hold,
			      get_cartoon_duration(&self->won) + 250000ULL);
	if (self->game->rewind)
		publish_game_renderer_info(&self->previous_level_info);
	else if (!game_level_completed(self->game))
		publish_game_renderer_info(&self->next_level_info);
	else
		publish_game_renderer_info(&self->level_complete_info);
}

static void on_level_failed(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	reset_cartoon(&self->lost, self->time);
	set_game_renderer_sprite_cartoon(&self->pacman, &self->lost);
	self->pacman_state = 2;
	if (!b6_event_is_pending(&self->hold.event))
		hold_game_for(self->game, &self->hold,
			      get_cartoon_duration(&self->lost) + 250000ULL);
	if (self->game->pacman.lifes < 0)
		publish_game_renderer_info(&self->game_over_info);
}

static void on_game_paused(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	switch (get_game_pause_reason(self->game)) {
	case GAME_PAUSED_READY:
		publish_game_renderer_info(&self->get_ready_info);
		break;
	case GAME_PAUSED_RETRY:
		publish_game_renderer_info(&self->try_again_info);
		break;
	case GAME_PAUSED_OTHER:
		publish_game_renderer_info(&self->paused_info);
		set_fade_io_target(&self->fade_io, .5f);
		break;
	}
}

static void on_game_resumed(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	unpublish_game_renderer_info(&self->get_ready_info);
	unpublish_game_renderer_info(&self->try_again_info);
	unpublish_game_renderer_info(&self->paused_info);
	set_fade_io_target(&self->fade_io, 1.f);
}

static void on_pacman_move(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	const struct mobile *mobile = &self->game->pacman.mobile;
	move_game_renderer_sprite(&self->pacman, mobile->x, mobile->y);
	self->pacman_direction = mobile->direction;
}

static void on_extra_life(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	set_game_renderer_count(&self->lifes, self->game->pacman.lifes);
	publish_game_renderer_info(&self->extra_life_info);
}

static void on_score_change(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	self->notified_score = self->game->pacman.score;
}

static void on_score_bump(struct game_observer *game_observer,
			  unsigned int amount)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	struct renderer_base *base = self->pacman.tile->parent;
	double x = base->x + 2;
	double y = base->y;
	amount /= 100;
	if (amount > 0 && amount <= b6_card_of(self->points))
		trigger_game_renderer_popup(&self->points[amount - 1], x, y);
}

static void on_booster_change(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	set_game_renderer_gauge(&self->booster,
				get_normalized_booster(self->game));
}

static void on_booster_small_reload(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	publish_game_renderer_info(&self->single_booster_info);
	on_booster_change(game_observer);
}

static void on_booster_large_reload(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	publish_game_renderer_info(&self->double_booster_info);
	on_booster_change(game_observer);
}

static void on_booster_full(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	publish_game_renderer_info(&self->full_booster_info);
}

static void on_booster_empty(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	publish_game_renderer_info(&self->empty_booster_info);
}

static void on_shield_change(struct game_observer *game_observer, int state)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	self->pacman_state = state;
	if (state > 0)
		publish_game_renderer_info(&self->bullet_proof_info);
	else if (state < 0)
		unpublish_game_renderer_info(&self->bullet_proof_info);
	set_game_renderer_count(&self->shields, self->game->pacman.shields);
}

static void on_shield_pickup(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	set_game_renderer_count(&self->shields, self->game->pacman.shields);
	publish_game_renderer_info(&self->shift_shield_info);
}

static void on_shield_empty(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	publish_game_renderer_info(&self->empty_shield_info);
}

static void on_jewel_pickup(struct game_observer *game_observer, int jewel)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	update_game_renderer_jewels(&self->jewels, self->game->pacman.jewels);
	publish_game_renderer_info(&self->jewel_info[jewel]);
}

static void on_ghost_move(struct game_observer *game_observer,
			  const struct ghost *ghost)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	move_game_renderer_sprite(&self->ghosts[ghost - self->game->ghosts],
				  ghost->mobile.x, ghost->mobile.y);
}

static void on_ghost_state_change(struct game_observer *game_observer,
				  const struct ghost *ghost)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	int n = ghost - self->game->ghosts;
	switch (get_ghost_state(ghost)) {
	case GHOST_ZOMBIE:
		self->ghosts_state[n] = 0;
		break;
	case GHOST_OUT:
		if (self->ghosts_state[n] == -1)
			break;
		set_game_renderer_sprite_cartoon(&self->ghosts[n], NULL);
		self->ghosts_state[n] = -1;
		break;
	default:
		self->ghosts_state[n] = 1;
	}
}

static void on_super_pacgum(struct game_observer *game_observer, int state)
{
	struct game_renderer * self = to_game_renderer(game_observer);
	self->afraid_state = state;
	if (state > 0)
		publish_game_renderer_info(&self->eat_the_ghosts_info);
	else if (state < 0)
		unpublish_game_renderer_info(&self->eat_the_ghosts_info);
}

static void on_zzz(struct game_observer *game_observer, int state)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	self->locked_state = state;
	if (state > 0)
		publish_game_renderer_info(&self->freeze_info);
	else if (state < 0)
		unpublish_game_renderer_info(&self->freeze_info);
}

static void on_x2_on(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	publish_game_renderer_info(&self->happy_hour_info);
}

static void on_x2_off(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	unpublish_game_renderer_info(&self->happy_hour_info);
}

static void on_diet_on(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	publish_game_renderer_info(&self->diet_info);
}

static void on_diet_off(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	unpublish_game_renderer_info(&self->diet_info);
}

static void on_slow_ghosts_on(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	publish_game_renderer_info(&self->slow_ghosts_info);
}

static void on_slow_ghosts_off(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	unpublish_game_renderer_info(&self->slow_ghosts_info);
}

static void on_fast_ghosts_on(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	publish_game_renderer_info(&self->fast_ghosts_info);
}

static void on_fast_ghosts_off(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	unpublish_game_renderer_info(&self->fast_ghosts_info);
}

static void on_slow_pacman_on(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	publish_game_renderer_info(&self->slow_pacman_info);
}

static void on_slow_pacman_off(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	unpublish_game_renderer_info(&self->slow_pacman_info);
}

static void on_fast_pacman_on(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	publish_game_renderer_info(&self->fast_pacman_info);
}

static void on_fast_pacman_off(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	unpublish_game_renderer_info(&self->fast_pacman_info);
}

static void on_wipeout(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	int i;
	for (i = 0; i < b6_card_of(self->ghosts); i += 1) {
		struct cartoon *cartoon = &self->wiped_out[i];
		if (self->ghosts_state[i] <= 0)
			continue;
		reset_cartoon(cartoon, self->time);
		self->ghosts_state[i] = -1;
		set_game_renderer_sprite_cartoon(&self->ghosts[i], cartoon);
	}
	publish_game_renderer_info(&self->wiped_out_info);
}

static void on_casino_launch(struct game_observer *game_observer)
{
	launch_game_renderer_casino(&to_game_renderer(game_observer)->casino);
}

static void on_casino_finish(struct game_observer *game_observer)
{
	finish_game_renderer_casino(&to_game_renderer(game_observer)->casino);
}

static void on_casino_update(struct game_observer *game_observer)
{
	struct game_renderer *self = to_game_renderer(game_observer);
	struct game_casino *casino = &self->game->casino;
	update_game_renderer_casino(&self->casino, casino);
}

static void game_renderer_on_render(struct renderer_observer *observer)
{
	struct game_renderer *self =
		b6_cast_of(observer, struct game_renderer, renderer_observer);
	int blink;
	unsigned long int n;
	struct cartoon *cartoon;
	struct toolkit_image *image;
	struct state *state;
	self->time = b6_get_clock_time(self->clock);
	blink = self->time / 250000 & 1;
	while ((state = current_state(&self->info_queue))) {
		struct game_renderer_info *info = to_game_renderer_info(state);
		if (!info->expire || info->expire > self->time)
			break;
		__dequeue_state(state, &self->info_queue);
	}
	if (self->pacman_state > 0)
		cartoon = &self->pacman_cartoons[1][self->pacman_direction];
	else if (self->pacman_state < 0)
		cartoon = &self->pacman_cartoons[0][self->pacman_direction];
	else
		cartoon = &self->pacman_cartoons[blink][self->pacman_direction];
	if (self->pacman_state <= 1)
		set_game_renderer_sprite_cartoon(&self->pacman, cartoon);
	for (n = 0; n < b6_card_of(self->ghosts_state); n += 1) {
		cartoon = &self->hunter[n];
		if (self->ghosts_state[n] < 0)
			continue;
		if (!self->ghosts_state[n])
			cartoon = &self->zombie;
		else if (self->locked_state >= 0) {
			if (self->locked_state || blink)
				cartoon = &self->locked;
			else if (self->afraid_state >= 0)
				cartoon = &self->afraid;
		} else if (self->afraid_state > 0 ||
			   (!self->afraid_state && blink))
			cartoon = &self->afraid;
		set_game_renderer_sprite_cartoon(&self->ghosts[n], cartoon);
	}
	if (self->time / 500000 & 1)
		for_each_gum(image, self->super_pacgums)
			hide_toolkit_image(image);
	else
		for_each_gum(image, self->super_pacgums)
			show_toolkit_image(image);
	if (self->rendered_score != self->notified_score) {
		struct b6_utf8 utf8;
		char s[] = "00000000";
		unsigned int score, i;
		self->rendered_score += 10;
		if (self->rendered_score > self->notified_score)
			self->rendered_score = self->notified_score;
		for (score = self->rendered_score, i = 8; i--;) {
			s[i] += score % 10;
			score /= 10;
		}
		set_toolkit_label(&self->score, b6_utf8_from_ascii(&utf8, s));
	}
}

static struct renderer_tile *create_playground(struct renderer *renderer,
					       struct renderer_base *parent)
{
	struct rgba rgba;
	struct renderer_base *base;
	struct renderer_tile *tile;
	struct renderer_texture *texture;
	base = create_renderer_base(renderer, parent, "playground", 0, 80);
	if (!base)
		goto fail_base;
	if (initialize_rgba(&rgba, LEVEL_WIDTH * 16, LEVEL_HEIGHT * 16))
		goto fail_texture;
	texture = create_renderer_texture(renderer, &rgba);
	finalize_rgba(&rgba);
	if (!texture)
		goto fail_texture;
	tile = create_renderer_tile(renderer, base, 0, 0,
				    LEVEL_WIDTH * 16, LEVEL_HEIGHT * 16,
				    texture);
	if (!tile)
		goto fail_tile;
	return tile;
fail_tile:
	destroy_renderer_texture(texture);
fail_texture:
	destroy_renderer_base(base);
fail_base:
	return NULL;
}

static void destroy_playground(struct renderer_tile *tile)
{
	destroy_renderer_texture(get_renderer_tile_texture(tile));
	destroy_renderer_base(get_renderer_tile_base(tile));
}

static int create_pacman(struct game_renderer *self, struct renderer_base *base)
{
	initialize_game_renderer_sprite(&self->pacman, self, base,
					0, 0, 32, 32);
	create_cartoon(&self->pacman_cartoons[0][LEVEL_E], self->renderer,
		       self->skin_id, GAME_PACMAN_DATA_ID("normal", "e"), 1);
	create_cartoon(&self->pacman_cartoons[0][LEVEL_W], self->renderer,
		       self->skin_id, GAME_PACMAN_DATA_ID("normal", "w"), 1);
	create_cartoon(&self->pacman_cartoons[0][LEVEL_S], self->renderer,
		       self->skin_id, GAME_PACMAN_DATA_ID("normal", "s"), 1);
	create_cartoon(&self->pacman_cartoons[0][LEVEL_N], self->renderer,
		       self->skin_id, GAME_PACMAN_DATA_ID("normal", "n"), 1);
	create_cartoon(&self->pacman_cartoons[1][LEVEL_E], self->renderer,
		       self->skin_id, GAME_PACMAN_DATA_ID("shield", "e"), 1);
	create_cartoon(&self->pacman_cartoons[1][LEVEL_W], self->renderer,
		       self->skin_id, GAME_PACMAN_DATA_ID("shield", "w"), 1);
	create_cartoon(&self->pacman_cartoons[1][LEVEL_S], self->renderer,
		       self->skin_id, GAME_PACMAN_DATA_ID("shield", "s"), 1);
	create_cartoon(&self->pacman_cartoons[1][LEVEL_N], self->renderer,
		       self->skin_id, GAME_PACMAN_DATA_ID("shield", "n"), 1);
	return 0;
}

static void destroy_pacman(struct game_renderer *self)
{
	enum direction d;
	for_each_direction(d) {
		destroy_cartoon(&self->pacman_cartoons[0][d]);
		destroy_cartoon(&self->pacman_cartoons[1][d]);
	}
	finalize_game_renderer_sprite(&self->pacman);
}

static int create_bonus(struct game_renderer *self, struct renderer_base *base)
{
	struct renderer *renderer = self->renderer;
	initialize_game_renderer_sprite(&self->bonus, self, base, 0, 0, 32, 32);
	create_cartoon(&self->bonus_cartoons[JEWELS_BONUS], renderer,
		       self->skin_id, GAME_BONUS_JEWELS_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[JEWEL1_BONUS], renderer,
		       self->skin_id, GAME_BONUS_JEWEL1_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[JEWEL2_BONUS], renderer,
		       self->skin_id, GAME_BONUS_JEWEL2_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[JEWEL3_BONUS], renderer,
		       self->skin_id, GAME_BONUS_JEWEL3_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[JEWEL4_BONUS], renderer,
		       self->skin_id, GAME_BONUS_JEWEL4_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[JEWEL5_BONUS], renderer,
		       self->skin_id, GAME_BONUS_JEWEL5_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[JEWEL6_BONUS], renderer,
		       self->skin_id, GAME_BONUS_JEWEL6_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[JEWEL7_BONUS], renderer,
		       self->skin_id, GAME_BONUS_JEWEL7_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[SINGLE_BOOSTER_BONUS], renderer,
		       self->skin_id, GAME_BONUS_SINGLE_BOOSTER_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[DOUBLE_BOOSTER_BONUS], renderer,
		       self->skin_id, GAME_BONUS_DOUBLE_BOOSTER_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[SUPER_PACGUM_BONUS], renderer,
		       self->skin_id, GAME_BONUS_SUPER_PACGUM_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[EXTRA_LIFE_BONUS], renderer,
		       self->skin_id, GAME_BONUS_LIFE_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[PACMAN_SLOW_BONUS], renderer,
		       self->skin_id, GAME_BONUS_PACMAN_SLOW_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[PACMAN_FAST_BONUS], renderer,
		       self->skin_id, GAME_BONUS_PACMAN_FAST_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[SHIELD_BONUS], renderer,
		       self->skin_id, GAME_BONUS_SHIELD_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[DIET_BONUS], renderer,
		       self->skin_id, GAME_BONUS_DIET_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[SUICIDE_BONUS], renderer,
		       self->skin_id, GAME_BONUS_DEATH_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[GHOSTS_BANQUET_BONUS], renderer,
		       self->skin_id, GAME_BONUS_GHOSTS_BANQUET_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[GHOSTS_WIPEOUT_BONUS], renderer,
		       self->skin_id, GAME_BONUS_GHOSTS_WIPEOUT_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[GHOSTS_SLOW_BONUS], renderer,
		       self->skin_id, GAME_BONUS_GHOSTS_SLOW_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[GHOSTS_FAST_BONUS], renderer,
		       self->skin_id, GAME_BONUS_GHOSTS_FAST_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[ZZZ_BONUS], renderer,
		       self->skin_id, GAME_BONUS_LOCKDOWN_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[X2_BONUS], renderer,
		       self->skin_id, GAME_BONUS_X2_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[MINUS_BONUS], renderer,
		       self->skin_id, GAME_BONUS_REWIND_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[PLUS_BONUS], renderer,
		       self->skin_id, GAME_BONUS_FORWARD_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[CASINO_BONUS], renderer,
		       self->skin_id, GAME_BONUS_CASINO_DATA_ID, 1);
	create_cartoon(&self->bonus_cartoons[SURPRISE_BONUS], renderer,
		       self->skin_id, GAME_BONUS_SURPRISE_DATA_ID, 1);
	return 0;
}

static void destroy_bonus(struct game_renderer *self)
{
	int i;
	for (i = 0; i < b6_card_of(self->bonus_cartoons); i += 1)
		destroy_cartoon(&self->bonus_cartoons[i]);
	finalize_game_renderer_sprite(&self->bonus);
}

static int create_ghosts(struct game_renderer *self,
			 struct renderer_base **base)
{
	int i;
	for (i = 0; i < b6_card_of(self->ghosts); i += 1) {
		static const char *data_id[] = {
			GAME_GHOST_DATA_ID(0, "normal", "n"),
			GAME_GHOST_DATA_ID(1, "normal", "n"),
			GAME_GHOST_DATA_ID(2, "normal", "n"),
			GAME_GHOST_DATA_ID(3, "normal", "n"),
		};
		initialize_game_renderer_sprite(&self->ghosts[i], self, *base++,
						0, 0, 32, 32);
		create_cartoon(&self->hunter[i], self->renderer, self->skin_id,
			       data_id[i], 1);
	}
	create_cartoon(&self->afraid, self->renderer, self->skin_id,
		       GAME_GHOST_DATA_ID(0, "afraid", "n"), 1);
	create_cartoon(&self->locked, self->renderer, self->skin_id,
		       GAME_GHOST_DATA_ID(0, "locked", "n"), 1);
	create_cartoon(&self->zombie, self->renderer, self->skin_id,
		       GAME_GHOST_DATA_ID(0, "zombie", "n"), 1);
	return 0;
}

static void destroy_ghosts(struct game_renderer *self)
{
	int i;
	destroy_cartoon(&self->zombie);
	destroy_cartoon(&self->locked);
	destroy_cartoon(&self->afraid);
	for (i = 0; i < b6_card_of(self->hunter); i += 1)
		destroy_cartoon(&self->hunter[i]);
	for (i = 0; i < b6_card_of(self->ghosts); i += 1)
		finalize_game_renderer_sprite(&self->ghosts[i]);
}

int create_points_popup(struct game_renderer *self, struct renderer_base **base)
{
	initialize_game_renderer_popup(&self->points[0], self, *base++,
				       GAME_POINTS_DATA_ID(100));
	initialize_game_renderer_popup(&self->points[1], self, *base++,
				       GAME_POINTS_DATA_ID(200));
	initialize_game_renderer_popup(&self->points[2], self, *base++,
				       GAME_POINTS_DATA_ID(300));
	initialize_game_renderer_popup(&self->points[3], self, *base++,
				       GAME_POINTS_DATA_ID(400));
	initialize_game_renderer_popup(&self->points[4], self, *base++,
				       GAME_POINTS_DATA_ID(500));
	initialize_game_renderer_popup(&self->points[5], self, *base++,
				       GAME_POINTS_DATA_ID(600));
	initialize_game_renderer_popup(&self->points[6], self, *base++,
				       GAME_POINTS_DATA_ID(700));
	initialize_game_renderer_popup(&self->points[7], self, *base++,
				       GAME_POINTS_DATA_ID(800));
	initialize_game_renderer_popup(&self->points[8], self, *base++,
				       GAME_POINTS_DATA_ID(900));
	return 0;
}

static void destroy_points_popup(struct game_renderer *self)
{
	int i;
	for (i = 0; i < b6_card_of(self->points); i += 1)
		finalize_game_renderer_popup(&self->points[i]);
}

static int create_panel(struct game_renderer *self, struct renderer_base *base,
			const struct b6_json_object *lang)
{
	static const struct b6_utf8 bonus_200 = B6_DEFINE_UTF8("bonus_200");
	struct rgba scratch;
	unsigned short int font_w, font_h;
	font_w = get_fixed_font_width(&self->font);
	font_h = get_fixed_font_height(&self->font);
	if ((self->top = create_renderer_tile(self->renderer, base,
					      0, 0, 640, 80, NULL)))
		set_renderer_tile_texture(self->top, make_texture(
				self->renderer, self->skin_id,
				GAME_PANEL_DATA_ID));
	initialize_toolkit_label(&self->level, self->renderer, &self->font,
				 3 * font_w + 2, font_h + 2, base,
				 6, 53, 50, 18);
	initialize_toolkit_label(&self->score, self->renderer, &self->font,
				 8 * font_w + 2, font_h + 2, base,
				 77, 53, 128, 18);
	self->info_tile = create_renderer_tile(self->renderer, base,
					       264, 53, 224, 18, NULL);
	self->info_icon_tile = create_renderer_tile(self->renderer, base,
						    222, 45, 32, 32, NULL);
	initialize_game_renderer_jewels(&self->jewels, self, base);
	initialize_rgba(&scratch, 224, 18);
	initialize_game_renderer_info(&self->get_ready_info, self, &scratch,
				      lang, B6_UTF8("get_ready"),
				      GAME_INFO_WIPEOUT_DATA_ID,
				      &sticky_info_ops);
	initialize_game_renderer_info(&self->paused_info, self, &scratch, lang,
				      B6_UTF8("paused"),
				      GAME_INFO_WIPEOUT_DATA_ID,
				      &sticky_info_ops);
	initialize_game_renderer_info(&self->try_again_info, self, &scratch,
				      lang, B6_UTF8("try_again"),
				      GAME_INFO_DEATH_DATA_ID,
				      &sticky_info_ops);
	initialize_game_renderer_info(&self->game_over_info, self, &scratch,
				      lang, B6_UTF8("game_over"),
				      GAME_INFO_DEATH_DATA_ID,
				      &sticky_info_ops);
	initialize_game_renderer_info(&self->happy_hour_info, self, &scratch,
				      lang, B6_UTF8("happy_hour"),
				      GAME_INFO_X2_DATA_ID, &standard_info_ops);
	initialize_game_renderer_info(&self->bullet_proof_info, self, &scratch,
				      lang, B6_UTF8("bullet_proof"),
				      GAME_INFO_INVINCIBLE_DATA_ID,
				      &standard_info_ops);
	initialize_game_renderer_info(&self->freeze_info, self, &scratch, lang,
				      B6_UTF8("freeze"),
				      GAME_INFO_LOCKDOWN_DATA_ID,
				      &standard_info_ops);
	initialize_game_renderer_info(&self->eat_the_ghosts_info, self,
				      &scratch, lang,
				      B6_UTF8("eat_the_ghosts"),
				      GAME_INFO_SUPER_PACGUM_DATA_ID,
				      &standard_info_ops);
	initialize_game_renderer_info(&self->extra_life_info, self, &scratch,
				      lang, B6_UTF8("extra_life"),
				      GAME_INFO_LIFE_DATA_ID,
				      &volatile_info_ops);
	initialize_game_renderer_info(&self->empty_shield_info, self, &scratch,
				      lang, B6_UTF8("empty_shield"),
				      GAME_INFO_EMPTY_DATA_ID,
				      &volatile_info_ops);
	initialize_game_renderer_info(&self->shift_shield_info, self, &scratch,
				      lang, B6_UTF8("shield_pickup"),
				      GAME_INFO_SHIELD_DATA_ID,
				      &volatile_info_ops);
	initialize_game_renderer_info(&self->level_complete_info, self,
				      &scratch, lang,
				      B6_UTF8("level_complete"),
				      GAME_INFO_COMPLETE_DATA_ID,
				      &sticky_info_ops);
	initialize_game_renderer_info(&self->next_level_info, self, &scratch,
				      lang, B6_UTF8("next_level"),
				      GAME_INFO_FORWARD_DATA_ID,
				      &sticky_info_ops);
	initialize_game_renderer_info(&self->previous_level_info, self,
				      &scratch, lang,
				      B6_UTF8("previous_level"),
				      GAME_INFO_REWIND_DATA_ID,
				      &sticky_info_ops);
	initialize_game_renderer_info(&self->slow_ghosts_info, self, &scratch,
				      lang, B6_UTF8("slow_ghosts"),
				      GAME_INFO_GHOSTS_SLOW_DATA_ID,
				      &standard_info_ops);
	initialize_game_renderer_info(&self->fast_ghosts_info, self, &scratch,
				      lang, B6_UTF8("fast_ghosts"),
				      GAME_INFO_GHOSTS_FAST_DATA_ID,
				      &standard_info_ops);
	initialize_game_renderer_info(&self->slow_pacman_info, self, &scratch,
				      lang, B6_UTF8("speed_slow"),
				      GAME_INFO_PACMAN_SLOW_DATA_ID,
				      &standard_info_ops);
	initialize_game_renderer_info(&self->fast_pacman_info, self, &scratch,
				      lang, B6_UTF8("speed_fast"),
				      GAME_INFO_PACMAN_FAST_DATA_ID,
				      &standard_info_ops);
	initialize_game_renderer_info(&self->single_booster_info, self,
				      &scratch, lang,
				      B6_UTF8("single_booster"),
				      GAME_INFO_SINGLE_BOOSTER_DATA_ID,
				      &volatile_info_ops);
	initialize_game_renderer_info(&self->double_booster_info, self,
				      &scratch, lang,
				      B6_UTF8("double_booster"),
				      GAME_INFO_DOUBLE_BOOSTER_DATA_ID,
				      &volatile_info_ops);
	initialize_game_renderer_info(&self->full_booster_info, self, &scratch,
				      lang, B6_UTF8("booster_full"),
				      GAME_INFO_DOUBLE_BOOSTER_DATA_ID,
				      &volatile_info_ops);
	initialize_game_renderer_info(&self->empty_booster_info, self, &scratch,
				      lang, B6_UTF8("booster_empty"),
				      GAME_INFO_EMPTY_DATA_ID,
				      &volatile_info_ops);
	initialize_game_renderer_info(&self->wiped_out_info, self, &scratch,
				      lang, B6_UTF8("wiped_out"),
				      GAME_INFO_WIPEOUT_DATA_ID,
				      &volatile_info_ops);
	initialize_game_renderer_info(&self->jewel_info[0], self, &scratch,
				     lang, &bonus_200, GAME_INFO_JEWEL1_DATA_ID,
				     &volatile_info_ops);
	initialize_game_renderer_info(&self->jewel_info[1], self, &scratch,
				     lang, &bonus_200, GAME_INFO_JEWEL2_DATA_ID,
				     &volatile_info_ops);
	initialize_game_renderer_info(&self->jewel_info[2], self, &scratch,
				     lang, &bonus_200, GAME_INFO_JEWEL3_DATA_ID,
				     &volatile_info_ops);
	initialize_game_renderer_info(&self->jewel_info[3], self, &scratch,
				     lang, &bonus_200, GAME_INFO_JEWEL4_DATA_ID,
				     &volatile_info_ops);
	initialize_game_renderer_info(&self->jewel_info[4], self, &scratch,
				     lang, &bonus_200, GAME_INFO_JEWEL5_DATA_ID,
				     &volatile_info_ops);
	initialize_game_renderer_info(&self->jewel_info[5], self, &scratch,
				     lang, &bonus_200, GAME_INFO_JEWEL6_DATA_ID,
				     &volatile_info_ops);
	initialize_game_renderer_info(&self->jewel_info[6], self, &scratch,
				     lang, &bonus_200, GAME_INFO_JEWEL7_DATA_ID,
				     &volatile_info_ops);
	initialize_game_renderer_info(&self->diet_info, self, &scratch,
				      lang, B6_UTF8("dieting"),
				      GAME_INFO_DIET_DATA_ID,
				      &standard_info_ops);
	initialize_game_renderer_casino(&self->casino, self, lang, &scratch);
	finalize_rgba(&scratch);
	if ((self->bottom[0] = create_renderer_tile(self->renderer, base,
						    0, 476, 4, 4, NULL)))
		set_renderer_tile_texture(self->bottom[0], make_texture(
				self->renderer, self->skin_id,
				GAME_BOTTOM_DATA_ID("left")));
	if ((self->bottom[1] = create_renderer_tile(self->renderer, base,
						    636, 476, 4, 4, NULL)))
		set_renderer_tile_texture(self->bottom[1], make_texture(
				self->renderer, self->skin_id,
				GAME_BOTTOM_DATA_ID("right")));
	initialize_game_renderer_gauge(&self->booster, self, base,
				       446, 13, 185, 10,
				       GAME_BOOSTER_DATA_ID);
	initialize_game_renderer_count(&self->lifes, self, base, 575, 53,
				       GAME_LIFE_ICON_DATA_ID);
	initialize_game_renderer_count(&self->shields, self, base, 503, 53,
				       GAME_SHIELD_ICON_DATA_ID);
	return 0;
}

static void destroy_panel(struct game_renderer *self)
{
	int i;
	finalize_game_renderer_info(&self->get_ready_info);
	finalize_game_renderer_info(&self->paused_info);
	finalize_game_renderer_info(&self->try_again_info);
	finalize_game_renderer_info(&self->game_over_info);
	finalize_game_renderer_info(&self->happy_hour_info);
	finalize_game_renderer_info(&self->bullet_proof_info);
	finalize_game_renderer_info(&self->freeze_info);
	finalize_game_renderer_info(&self->eat_the_ghosts_info);
	finalize_game_renderer_info(&self->extra_life_info);
	finalize_game_renderer_info(&self->empty_shield_info);
	finalize_game_renderer_info(&self->shift_shield_info);
	finalize_game_renderer_info(&self->level_complete_info);
	finalize_game_renderer_info(&self->next_level_info);
	finalize_game_renderer_info(&self->previous_level_info);
	finalize_game_renderer_info(&self->slow_ghosts_info);
	finalize_game_renderer_info(&self->fast_ghosts_info);
	finalize_game_renderer_info(&self->slow_pacman_info);
	finalize_game_renderer_info(&self->fast_pacman_info);
	finalize_game_renderer_info(&self->single_booster_info);
	finalize_game_renderer_info(&self->double_booster_info);
	finalize_game_renderer_info(&self->full_booster_info);
	finalize_game_renderer_info(&self->empty_booster_info);
	finalize_game_renderer_info(&self->wiped_out_info);
	for (i = 0; i < b6_card_of(self->jewel_info); i += 1)
		finalize_game_renderer_info(&self->jewel_info[i]);
	finalize_game_renderer_info(&self->diet_info);
	finalize_toolkit_label(&self->score);
	finalize_toolkit_label(&self->level);
	finalize_game_renderer_count(&self->lifes);
	finalize_game_renderer_count(&self->shields);
	destroy_renderer_tile(self->info_icon_tile);
	destroy_renderer_tile(self->info_tile);
	finalize_game_renderer_casino(&self->casino);
	finalize_game_renderer_gauge(&self->booster);
	finalize_game_renderer_jewels(&self->jewels);
	if (self->top) {
		destroy_renderer_texture(get_renderer_tile_texture(self->top));
		destroy_renderer_tile(self->top);
	}
	for (i = 0; i < b6_card_of(self->bottom); i += 1)
		if (self->bottom[i]) {
			destroy_renderer_texture(self->bottom[i]->texture);
			destroy_renderer_tile(self->bottom[i]);
		}
}

int initialize_game_renderer(struct game_renderer *self,
			     struct renderer *renderer,
			     const struct b6_clock *clock, struct game *game,
			     const char *skin_id,
			     const struct b6_json_object *lang)
{
	static const struct game_observer_ops game_observer_ops = {
		.on_game_paused = on_game_paused,
		.on_game_resumed = on_game_resumed,
		.on_level_init = on_level_init,
		.on_level_exit = on_level_exit,
		.on_level_enter = on_level_enter,
		.on_level_leave = on_level_leave,
		.on_level_start = on_level_start,
		.on_level_touch = on_level_touch,
		.on_level_passed = on_level_passed,
		.on_level_failed = on_level_failed,
		.on_pacman_move = on_pacman_move,
		.on_ghost_move = on_ghost_move,
		.on_ghost_state_change = on_ghost_state_change,
		.on_score_change = on_score_change,
		.on_score_bump = on_score_bump,
		.on_booster_charge = on_booster_change,
		.on_booster_micro_reload = on_booster_change,
		.on_booster_small_reload = on_booster_small_reload,
		.on_booster_large_reload = on_booster_large_reload,
		.on_booster_full = on_booster_full,
		.on_booster_empty = on_booster_empty,
		.on_extra_life = on_extra_life,
		.on_shield_change = on_shield_change,
		.on_shield_pickup = on_shield_pickup,
		.on_shield_empty = on_shield_empty,
		.on_jewel_pickup = on_jewel_pickup,
		.on_wipeout = on_wipeout,
		.on_super_pacgum = on_super_pacgum,
		.on_zzz = on_zzz,
		.on_x2_on = on_x2_on,
		.on_x2_off = on_x2_off,
		.on_pacman_diet_begin = on_diet_on,
		.on_pacman_diet_end = on_diet_off,
		.on_slow_ghosts_on = on_slow_ghosts_on,
		.on_slow_ghosts_off = on_slow_ghosts_off,
		.on_fast_ghosts_on = on_fast_ghosts_on,
		.on_fast_ghosts_off = on_fast_ghosts_off,
		.on_slow_pacman_on = on_slow_pacman_on,
		.on_slow_pacman_off = on_slow_pacman_off,
		.on_fast_pacman_on = on_fast_pacman_on,
		.on_fast_pacman_off = on_fast_pacman_off,
		.on_casino_launch = on_casino_launch,
		.on_casino_update = on_casino_update,
		.on_casino_finish = on_casino_finish,
	};
	static const struct renderer_observer_ops renderer_observer_ops = {
		.on_render = game_renderer_on_render,
	};
	struct renderer_base *root = get_renderer_base(renderer);
	struct renderer_base *playground_base, *ghost_base[4], *pacman_base,
			     *bonus_base, *points_popup_base[9], *panel_base;
	int i;
	self->renderer = renderer;
	self->clock = clock;
	self->game = game;
	self->skin_id = skin_id;
	reset_state_queue(&self->info_queue);
	self->notified_score = 0;
	initialize_fade_io(&self->fade_io, "game_fade_io", renderer, clock,
			   0.f, 0.f, 2e-6f);
	for (i = 0; i < b6_card_of(self->ghosts_state); i += 1)
		self->ghosts_state[i] = -1;
	setup_game_observer(&self->game_observer, &game_observer_ops);
	setup_renderer_observer(&self->renderer_observer, "game_renderer",
				&renderer_observer_ops);
	if (!(self->playground = create_playground(renderer, root)))
		return -1;
	if (make_font(&self->font, skin_id, GAME_FONT_DATA_ID)) {
		destroy_playground(self->playground);
		return -1;
	}
	playground_base = get_renderer_tile_base(self->playground);
	ghost_base[0] = create_renderer_base(
		renderer, playground_base, "ghost_1", 0, 0);
	ghost_base[1] = create_renderer_base(
		renderer, playground_base, "ghost_2", 0, 0);
	ghost_base[2] = create_renderer_base(
		renderer, playground_base, "ghost_3", 0, 0);
	ghost_base[3] = create_renderer_base(
		renderer, playground_base, "ghost_4", 0, 0);
	bonus_base = create_renderer_base_or_die(
		renderer, playground_base, "bonus", 0, 0);
	pacman_base = create_renderer_base_or_die(
		renderer, playground_base, "pacman", 0, 0);
	for (i = 0; i < b6_card_of(points_popup_base); i += 1)
		points_popup_base[i] = create_renderer_base(
			renderer, playground_base, "points", 0, 0);
	panel_base = create_renderer_base(renderer, playground_base, "panel",
					  0, -80);
	create_cartoon(&self->won, renderer, skin_id,
		       GAME_PACMAN_WINS_DATA_ID, 0);
	create_cartoon(&self->lost, renderer, skin_id,
		       GAME_PACMAN_LOSES_DATA_ID, 0);
	for (i = 0; i < b6_card_of(self->ghosts); i += 1)
		copy_cartoon(&self->lost, &self->wiped_out[i]);
	create_bonus(self, bonus_base);
	create_panel(self, panel_base, lang);
	create_pacman(self, pacman_base);
	create_ghosts(self, ghost_base);
	create_points_popup(self, points_popup_base);
	self->pacgum_texture = make_texture(renderer, skin_id,
					    GAME_PACGUM_DATA_ID);
	self->super_pacgum_texture = make_texture(renderer, skin_id,
						  GAME_SUPER_PACGUM_DATA_ID);
	add_game_observer(self->game, &self->game_observer);
	add_renderer_observer(self->renderer, &self->renderer_observer);
	return 0;
}

void finalize_game_renderer(struct game_renderer *self)
{
	del_renderer_observer(&self->renderer_observer);
	del_game_observer(&self->game_observer);
	destroy_renderer_texture(self->super_pacgum_texture);
	destroy_renderer_texture(self->pacgum_texture);
	destroy_points_popup(self);
	destroy_ghosts(self);
	destroy_pacman(self);
	destroy_panel(self);
	destroy_bonus(self);
	destroy_cartoon(&self->lost);
	destroy_cartoon(&self->won);
	destroy_playground(self->playground);
	finalize_fade_io(&self->fade_io);
	finalize_fixed_font(&self->font);
}
