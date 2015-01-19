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

#include "menu_renderer.h"

#include <b6/allocator.h>
#include <b6/clock.h>
#include <b6/json.h>
#include <b6/utf8.h>

#include "lib/std.h"
#include "menu_phase.h"
#include "data.h"

/* 512 half-degree cos table + 64 values for sin */
const float _cosqf[] = {
	 1.000000f,  0.999925f,  0.999699f,  0.999322f,
	 0.998795f,  0.998118f,  0.997290f,  0.996313f,
	 0.995185f,  0.993907f,  0.992480f,  0.990903f,
	 0.989177f,  0.987301f,  0.985278f,  0.983105f,
	 0.980785f,  0.978317f,  0.975702f,  0.972940f,
	 0.970031f,  0.966976f,  0.963776f,  0.960431f,
	 0.956940f,  0.953306f,  0.949528f,  0.945607f,
	 0.941544f,  0.937339f,  0.932993f,  0.928506f,
	 0.923880f,  0.919114f,  0.914210f,  0.909168f,
	 0.903989f,  0.898674f,  0.893224f,  0.887640f,
	 0.881921f,  0.876070f,  0.870087f,  0.863973f,
	 0.857729f,  0.851355f,  0.844854f,  0.838225f,
	 0.831470f,  0.824589f,  0.817585f,  0.810457f,
	 0.803208f,  0.795837f,  0.788346f,  0.780737f,
	 0.773010f,  0.765167f,  0.757209f,  0.749136f,
	 0.740951f,  0.732654f,  0.724247f,  0.715731f,
	 0.707107f,  0.698376f,  0.689541f,  0.680601f,
	 0.671559f,  0.662416f,  0.653173f,  0.643832f,
	 0.634393f,  0.624860f,  0.615232f,  0.605511f,
	 0.595699f,  0.585798f,  0.575808f,  0.565732f,
	 0.555570f,  0.545325f,  0.534998f,  0.524590f,
	 0.514103f,  0.503538f,  0.492898f,  0.482184f,
	 0.471397f,  0.460539f,  0.449611f,  0.438616f,
	 0.427555f,  0.416430f,  0.405241f,  0.393992f,
	 0.382683f,  0.371317f,  0.359895f,  0.348419f,
	 0.336890f,  0.325310f,  0.313682f,  0.302006f,
	 0.290285f,  0.278520f,  0.266713f,  0.254866f,
	 0.242980f,  0.231058f,  0.219101f,  0.207111f,
	 0.195090f,  0.183040f,  0.170962f,  0.158858f,
	 0.146730f,  0.134581f,  0.122411f,  0.110222f,
	 0.098017f,  0.085797f,  0.073565f,  0.061321f,
	 0.049068f,  0.036807f,  0.024541f,  0.012272f,
	-0.000000f, -0.012271f, -0.024541f, -0.036807f,
	-0.049068f, -0.061321f, -0.073565f, -0.085797f,
	-0.098017f, -0.110222f, -0.122411f, -0.134581f,
	-0.146730f, -0.158858f, -0.170962f, -0.183040f,
	-0.195090f, -0.207111f, -0.219101f, -0.231058f,
	-0.242980f, -0.254866f, -0.266713f, -0.278520f,
	-0.290285f, -0.302006f, -0.313682f, -0.325310f,
	-0.336890f, -0.348419f, -0.359895f, -0.371317f,
	-0.382683f, -0.393992f, -0.405241f, -0.416430f,
	-0.427555f, -0.438616f, -0.449611f, -0.460539f,
	-0.471397f, -0.482184f, -0.492898f, -0.503538f,
	-0.514103f, -0.524590f, -0.534998f, -0.545325f,
	-0.555570f, -0.565732f, -0.575808f, -0.585798f,
	-0.595699f, -0.605511f, -0.615232f, -0.624859f,
	-0.634393f, -0.643832f, -0.653173f, -0.662416f,
	-0.671559f, -0.680601f, -0.689540f, -0.698376f,
	-0.707107f, -0.715731f, -0.724247f, -0.732654f,
	-0.740951f, -0.749136f, -0.757209f, -0.765167f,
	-0.773010f, -0.780737f, -0.788346f, -0.795837f,
	-0.803207f, -0.810457f, -0.817585f, -0.824589f,
	-0.831470f, -0.838225f, -0.844854f, -0.851355f,
	-0.857729f, -0.863973f, -0.870087f, -0.876070f,
	-0.881921f, -0.887640f, -0.893224f, -0.898674f,
	-0.903989f, -0.909168f, -0.914210f, -0.919114f,
	-0.923880f, -0.928506f, -0.932993f, -0.937339f,
	-0.941544f, -0.945607f, -0.949528f, -0.953306f,
	-0.956940f, -0.960431f, -0.963776f, -0.966976f,
	-0.970031f, -0.972940f, -0.975702f, -0.978317f,
	-0.980785f, -0.983105f, -0.985278f, -0.987301f,
	-0.989176f, -0.990903f, -0.992480f, -0.993907f,
	-0.995185f, -0.996313f, -0.997290f, -0.998118f,
	-0.998795f, -0.999322f, -0.999699f, -0.999925f,
	-1.000000f, -0.999925f, -0.999699f, -0.999322f,
	-0.998795f, -0.998118f, -0.997290f, -0.996313f,
	-0.995185f, -0.993907f, -0.992480f, -0.990903f,
	-0.989177f, -0.987301f, -0.985278f, -0.983106f,
	-0.980785f, -0.978317f, -0.975702f, -0.972940f,
	-0.970031f, -0.966976f, -0.963776f, -0.960431f,
	-0.956940f, -0.953306f, -0.949528f, -0.945607f,
	-0.941544f, -0.937339f, -0.932993f, -0.928506f,
	-0.923880f, -0.919114f, -0.914210f, -0.909168f,
	-0.903989f, -0.898674f, -0.893224f, -0.887640f,
	-0.881921f, -0.876070f, -0.870087f, -0.863973f,
	-0.857729f, -0.851355f, -0.844854f, -0.838225f,
	-0.831470f, -0.824589f, -0.817585f, -0.810457f,
	-0.803208f, -0.795837f, -0.788346f, -0.780737f,
	-0.773011f, -0.765167f, -0.757209f, -0.749136f,
	-0.740951f, -0.732654f, -0.724247f, -0.715731f,
	-0.707107f, -0.698376f, -0.689541f, -0.680601f,
	-0.671559f, -0.662416f, -0.653173f, -0.643832f,
	-0.634393f, -0.624859f, -0.615232f, -0.605511f,
	-0.595699f, -0.585798f, -0.575808f, -0.565732f,
	-0.555570f, -0.545325f, -0.534998f, -0.524590f,
	-0.514103f, -0.503538f, -0.492898f, -0.482184f,
	-0.471397f, -0.460539f, -0.449611f, -0.438616f,
	-0.427555f, -0.416429f, -0.405242f, -0.393992f,
	-0.382684f, -0.371317f, -0.359895f, -0.348419f,
	-0.336890f, -0.325310f, -0.313682f, -0.302006f,
	-0.290285f, -0.278520f, -0.266713f, -0.254865f,
	-0.242980f, -0.231058f, -0.219101f, -0.207111f,
	-0.195090f, -0.183040f, -0.170962f, -0.158858f,
	-0.146730f, -0.134581f, -0.122411f, -0.110222f,
	-0.098017f, -0.085798f, -0.073565f, -0.061321f,
	-0.049068f, -0.036807f, -0.024541f, -0.012272f,
	 0.000000f,  0.012272f,  0.024541f,  0.036807f,
	 0.049068f,  0.061321f,  0.073565f,  0.085797f,
	 0.098017f,  0.110222f,  0.122411f,  0.134581f,
	 0.146730f,  0.158858f,  0.170962f,  0.183040f,
	 0.195090f,  0.207112f,  0.219101f,  0.231058f,
	 0.242980f,  0.254865f,  0.266713f,  0.278520f,
	 0.290285f,  0.302006f,  0.313682f,  0.325310f,
	 0.336890f,  0.348419f,  0.359895f,  0.371317f,
	 0.382684f,  0.393992f,  0.405241f,  0.416429f,
	 0.427555f,  0.438616f,  0.449611f,  0.460539f,
	 0.471397f,  0.482184f,  0.492898f,  0.503538f,
	 0.514103f,  0.524590f,  0.534998f,  0.545325f,
	 0.555570f,  0.565732f,  0.575808f,  0.585798f,
	 0.595699f,  0.605511f,  0.615232f,  0.624860f,
	 0.634393f,  0.643832f,  0.653173f,  0.662416f,
	 0.671559f,  0.680601f,  0.689540f,  0.698376f,
	 0.707107f,  0.715731f,  0.724247f,  0.732654f,
	 0.740951f,  0.749136f,  0.757209f,  0.765167f,
	 0.773011f,  0.780737f,  0.788347f,  0.795837f,
	 0.803207f,  0.810457f,  0.817585f,  0.824589f,
	 0.831470f,  0.838225f,  0.844854f,  0.851355f,
	 0.857729f,  0.863973f,  0.870087f,  0.876070f,
	 0.881921f,  0.887640f,  0.893224f,  0.898674f,
	 0.903989f,  0.909168f,  0.914210f,  0.919114f,
	 0.923880f,  0.928506f,  0.932993f,  0.937339f,
	 0.941544f,  0.945607f,  0.949528f,  0.953306f,
	 0.956940f,  0.960430f,  0.963776f,  0.966976f,
	 0.970031f,  0.972940f,  0.975702f,  0.978317f,
	 0.980785f,  0.983106f,  0.985278f,  0.987301f,
	 0.989177f,  0.990903f,  0.992480f,  0.993907f,
	 0.995185f,  0.996313f,  0.997290f,  0.998118f,
	 0.998795f,  0.999322f,  0.999699f,  0.999925f,
	 0.000000f,  0.012272f,  0.024541f,  0.036807f,
	 0.049068f,  0.061321f,  0.073565f,  0.085797f,
	 0.098017f,  0.110222f,  0.122411f,  0.134581f,
	 0.146730f,  0.158858f,  0.170962f,  0.183040f,
	 0.195090f,  0.207111f,  0.219101f,  0.231058f,
	 0.242980f,  0.254866f,  0.266713f,  0.278520f,
	 0.290285f,  0.302006f,  0.313682f,  0.325310f,
	 0.336890f,  0.348419f,  0.359895f,  0.371317f,
	 0.382683f,  0.393992f,  0.405241f,  0.416430f,
	 0.427555f,  0.438616f,  0.449611f,  0.460539f,
	 0.471397f,  0.482184f,  0.492898f,  0.503538f,
	 0.514103f,  0.524590f,  0.534998f,  0.545325f,
	 0.555570f,  0.565732f,  0.575808f,  0.585798f,
	 0.595699f,  0.605511f,  0.615232f,  0.624860f,
	 0.634393f,  0.643832f,  0.653173f,  0.662416f,
	 0.671559f,  0.680601f,  0.689541f,  0.698376f
};

static inline float cosqf(unsigned long int a) { return _cosqf[a & 511]; }

static inline float sinqf(unsigned long int a) { return cosqf(a + 64); }

static void menu_entry_renderer_on_render(struct renderer_observer *obs)
{
	struct menu_entry_renderer *self =
		b6_cast_of(obs, struct menu_entry_renderer, renderer_observer);
	unsigned long long int now = b6_get_clock_time(self->clock);
	int selected = now & 262144;
	show_toolkit_label(&self->label[!!selected]);
	hide_toolkit_label(&self->label[!selected]);
}

static unsigned int text_len(const unsigned char *utf8, unsigned int size)
{
	unsigned int len;
	for (len = 0; size; len += 1) {
		int n = b6_utf8_dec_len(utf8);
		if (n <= 0)
			break;
		if (n > size)
			break;
		utf8 += n;
		size -= n;
	}
	return len;
}

static void initialize_menu_entry_renderer(struct menu_entry_renderer *self,
					   const struct b6_clock *clock,
					   struct renderer *renderer,
					   struct renderer_base *base,
					   const struct fixed_font *normal_font,
					   const struct fixed_font *bright_font,
					   float x, float y,
					   const void *utf8_data,
					   unsigned int utf8_size)
{
	static const struct renderer_observer_ops renderer_observer_ops = {
		.on_render = menu_entry_renderer_on_render,
	};
	unsigned long int len = text_len(utf8_data, utf8_size);
	unsigned short int font_w = get_fixed_font_width(normal_font);
	unsigned short int font_h = get_fixed_font_height(normal_font);
	initialize_toolkit_label(&self->label[0], renderer, normal_font,
				 len * font_w, font_h, base,
				 x - len * 16 / 2, y, len * 16, 16);
	set_toolkit_label_utf8(&self->label[0], utf8_data, utf8_size);
	enable_toolkit_label_shadow(&self->label[0]);
	initialize_toolkit_label(&self->label[1], renderer, bright_font,
				 len * font_w, font_h, base,
				 x - len * 16 / 2, y, len * 16, 16);
	set_toolkit_label_utf8(&self->label[1], utf8_data, utf8_size);
	enable_toolkit_label_shadow(&self->label[1]);
	hide_toolkit_label(&self->label[1]);
	self->clock = clock;
	setup_renderer_observer(&self->renderer_observer, "menu_entry",
				&renderer_observer_ops);
}

static void finalize_menu_entry_renderer(struct menu_entry_renderer *self)
{
	finalize_toolkit_label(&self->label[1]);
	finalize_toolkit_label(&self->label[0]);
}

static void move_shadow(struct menu_renderer_image *self)
{
	unsigned long int a = (self->linear.time % 2097152) / 4096;
	float dx = sinqf(2 * a) * 3 - 2;
	float dy = cosqf(a) * 3 + 6;
	move_renderer_base(self->tile[1]->parent, dx, dy);
}

static void menu_renderer_image_on_render_h(struct renderer_observer *observer)
{
	struct menu_renderer_image *self = b6_cast_of(
		observer, struct menu_renderer_image, renderer_observer);
	self->base->x = update_linear(&self->linear);
	move_shadow(self);
}

static void menu_renderer_image_on_render_v(struct renderer_observer *observer)
{
	struct menu_renderer_image *self = b6_cast_of(
		observer, struct menu_renderer_image, renderer_observer);
	self->base->y = update_linear(&self->linear);
	move_shadow(self);
}

static int initialize_menu_renderer_image(
	struct menu_renderer_image *self, struct renderer *renderer,
	const struct b6_clock *clock, float x, float y, float w, float h,
	float pos, float speed, const char *skin_id, const char *data_id,
	const struct renderer_observer_ops *ops)
{
	struct renderer_base *root = get_renderer_base(renderer);
	struct renderer_texture *texture[2];
	struct data_entry *entry;
	struct image_data *data;
	struct rgba rgba;
	self->base = NULL;
	if (get_image_data(skin_id, data_id, NULL, &entry, &data))
		return -1;
	initialize_rgba(&rgba, data->w, data->h);
	copy_rgba(data->rgba, *data->x, *data->y, data->w, data->h,
		  &rgba, 0, 0);
	texture[0] = create_renderer_texture_or_die(renderer, &rgba);
	make_shadow_rgba(&rgba);
	texture[1] = create_renderer_texture_or_die(renderer, &rgba);
	finalize_rgba(&rgba);
	self->base = create_renderer_base_or_die(renderer, root, "base", x, y);
	self->tile[1] = create_renderer_tile_or_die(
		renderer, create_renderer_base_or_die(renderer, self->base,
						      "shadow", 3, 3),
		0, 0, w, h, texture[1]);
	self->tile[0] = create_renderer_tile_or_die(
		renderer, create_renderer_base_or_die(renderer, self->base,
						      "image", 0, 0),
		0, 0, w, h, texture[0]);
	put_image_data(entry, data);
	if (ops->on_render == menu_renderer_image_on_render_v)
		setup_linear(&self->linear, clock, self->base->y, pos, speed);
	else
		setup_linear(&self->linear, clock, self->base->x, pos, speed);
	add_renderer_observer(renderer, setup_renderer_observer(
			&self->renderer_observer, data_id, ops));
	return 0;
}

static void finalize_menu_renderer_image(struct menu_renderer_image *self)
{
	if (self->base) {
		del_renderer_observer(&self->renderer_observer);
		destroy_renderer_texture(self->tile[1]->texture);
		destroy_renderer_texture(self->tile[0]->texture);
		destroy_renderer_base(self->base);
	}
}

static void menu_renderer_on_render(struct renderer_observer *observer)
{
	struct menu_renderer *self =
		b6_cast_of(observer, struct menu_renderer, renderer_observer);
	dim_renderer(self->renderer, 1.f);
}

static struct menu_entry_renderer *get_current_menu_entry_renderer(
	struct menu_renderer *self)
{
	return get_current_menu_entry(self->menu)->cookie;
}

static void menu_renderer_on_focus_in(struct menu_observer *observer)
{
	struct menu_renderer *self =
		b6_cast_of(observer, struct menu_renderer, menu_observer);
	struct menu_entry_renderer *e = get_current_menu_entry_renderer(self);
	add_renderer_observer(self->renderer, &e->renderer_observer);
}

static void menu_renderer_on_focus_out(struct menu_observer *observer)
{
	struct menu_renderer *self =
		b6_cast_of(observer, struct menu_renderer, menu_observer);
	struct menu_entry_renderer *e = get_current_menu_entry_renderer(self);
	del_renderer_observer(&e->renderer_observer);
	show_toolkit_label(&e->label[0]);
	hide_toolkit_label(&e->label[1]);
}

static int initialize_menu_renderer_entries(struct menu_renderer *self)
{
	float x = 320;
	float y = 256 - 60;
	struct menu_iterator iter;
	struct renderer_base *root = get_renderer_base(self->renderer);
	setup_menu_iterator(&iter, self->menu);
	while (menu_iterator_has_next(&iter)) {
		struct menu_entry *entry = menu_iterator_next(&iter);
		initialize_menu_entry_renderer(entry->cookie, self->clock,
					       self->renderer, root,
					       &self->normal_font,
					       &self->bright_font,
					       x, y,
					       entry->utf8_data,
					       entry->utf8_size);
		y += 40;
	}
	return 0;
}

static void finalize_menu_renderer_entries(struct menu_renderer *self)
{
	struct menu_iterator iter;
	setup_menu_iterator(&iter, self->menu);
	while (menu_iterator_has_next(&iter)) {
		struct menu_entry *entry = menu_iterator_next(&iter);
		finalize_menu_entry_renderer(entry->cookie);
	}
}

void update_menu_renderer(struct menu_renderer *self)
{
	if (!self->menu)
		return;
	menu_renderer_on_focus_out(&self->menu_observer);
	finalize_menu_renderer_entries(self);
	initialize_menu_renderer_entries(self);
	menu_renderer_on_focus_in(&self->menu_observer);
}

void open_menu_renderer(struct menu_renderer *self, struct menu *menu)
{
	static const struct menu_observer_ops menu_observer_ops = {
		.on_focus_in = menu_renderer_on_focus_in,
		.on_focus_out = menu_renderer_on_focus_out,
	};
	struct menu_iterator iter;
	self->menu = menu;
	setup_menu_iterator(&iter, menu);
	while (menu_iterator_has_next(&iter)) {
		struct menu_entry *entry = menu_iterator_next(&iter);
		entry->cookie = b6_allocate(&b6_std_allocator,
					    sizeof(struct menu_entry_renderer));
	}
	initialize_menu_renderer_entries(self);
	setup_menu_observer(&self->menu_observer, &menu_observer_ops);
	add_menu_observer(self->menu, &self->menu_observer);
	menu_renderer_on_focus_in(&self->menu_observer);
}

void close_menu_renderer(struct menu_renderer *self)
{
	struct menu_iterator iter;
	menu_renderer_on_focus_out(&self->menu_observer);
	del_menu_observer(&self->menu_observer);
	finalize_menu_renderer_entries(self);
	setup_menu_iterator(&iter, self->menu);
	while (menu_iterator_has_next(&iter)) {
		struct menu_entry *entry = menu_iterator_next(&iter);
		b6_deallocate(&b6_std_allocator, entry->cookie);
	}
	self->menu = NULL;
}

int initialize_menu_renderer(struct menu_renderer *self,
			     struct renderer *renderer,
			     const struct b6_clock *clock,
			     const char *skin_id)
{
	static const struct renderer_observer_ops h_renderer_observer_ops = {
		.on_render = menu_renderer_image_on_render_h,
	};
	static const struct renderer_observer_ops v_renderer_observer_ops = {
		.on_render = menu_renderer_image_on_render_v,
	};
	static const struct renderer_observer_ops renderer_observer_ops = {
		.on_render = menu_renderer_on_render,
	};
	struct renderer_base *root;
	self->renderer = renderer;
	self->clock = clock;
	self->menu = NULL;
	root = get_renderer_base(renderer);
	if (make_font(&self->normal_font, skin_id, MENU_NORMAL_FONT_DATA_ID))
		return -1;
	make_font(&self->bright_font, skin_id, MENU_BRIGHT_FONT_DATA_ID);
	if ((self->background = create_renderer_tile(renderer, root, 0, 0,
						     640, 480, NULL)))
		set_renderer_tile_texture(self->background, make_texture(
			renderer, skin_id, MENU_BACKGROUND_DATA_ID));
	initialize_menu_renderer_image(&self->title, renderer, self->clock,
				       70, -118, 500, 150, 32, 3e-4,
				       skin_id, MENU_TITLE_DATA_ID,
				       &v_renderer_observer_ops);
	initialize_menu_renderer_image(&self->pacman, renderer, self->clock,
				       640, 328, 96, 96, 440, 4e-4,
				       skin_id, MENU_PACMAN_DATA_ID,
				       &h_renderer_observer_ops);
	initialize_menu_renderer_image(&self->ghost, renderer, self->clock,
				       -100, 304, 100, 124, 100, 4e-4,
				       skin_id, MENU_GHOST_DATA_ID,
				       &h_renderer_observer_ops);
	add_renderer_observer(self->renderer, setup_renderer_observer(
			&self->renderer_observer,
			"menu_renderer",
			&renderer_observer_ops));
	return 0;
}

void finalize_menu_renderer(struct menu_renderer *self)
{
	del_renderer_observer(&self->renderer_observer);
	finalize_menu_renderer_image(&self->ghost);
	finalize_menu_renderer_image(&self->pacman);
	finalize_menu_renderer_image(&self->title);
	if (self->background) {
		destroy_renderer_texture(self->background->texture);
		destroy_renderer_tile(self->background);
	}
	finalize_fixed_font(&self->bright_font);
	finalize_fixed_font(&self->normal_font);
}
