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

#include "hall_of_fame_phase.h"

#include <b6/allocator.h>
#include <b6/clock.h>
#include <b6/cmdline.h>
#include <b6/utf8.h>

#include "console.h"
#include "controller.h"
#include "data.h"
#include "engine.h"
#include "fade_io.h"
#include "game.h"
#include "hall_of_fame.h"
#include "lib/init.h"
#include "mixer.h"
#include "renderer.h"
#include "toolkit.h"

static const char *hof_skin = NULL;
b6_flag(hof_skin, string);

struct hall_of_fame_phase {
	struct phase up;
	struct b6_json_array *array;
	struct b6_json_object *entry;
	char buffer[64];
	struct b6_fixed_allocator fixed_allocator;
	struct b6_utf8_string name;
	unsigned long int rank;
	struct controller_observer controller_observer;
	short int music;
	short int quit;
	struct renderer_observer renderer_observer;
	struct renderer_tile *background;
	struct renderer_tile *panel;
	struct toolkit_label label[10];
	struct fixed_font font;
	struct toolkit_label cursor_label;
	struct renderer_base *cursor_base;
	struct fade_io fade_io;
};

static const char *user = NULL;
b6_flag(user, string);

static void u32_to_str(char *str, unsigned long int num, int len)
{
	while (len--) {
		str[len] = '0' + (num % 10);
		num /= 10;
	}
}

static void setup_label(struct hall_of_fame_phase *self, int n,
			struct b6_json_object *entry)
{
	char text[] = "00 000 00000000 ";
	unsigned int level, score;
	const struct b6_utf8 *label;
	struct b6_utf8 utf8;
	char buf[64];
	struct b6_fixed_allocator fixed_allocator;
	struct b6_utf8_string s;
	b6_reset_fixed_allocator(&fixed_allocator, buf, sizeof(buf));
	b6_initialize_utf8_string(&s, &fixed_allocator.allocator);
	u32_to_str(&text[0], n + 1, 2);
	if (!split_hall_of_fame_entry(entry, &level, &score, &label)) {
		u32_to_str(&text[3], level, 3);
		u32_to_str(&text[7], score, 8);
		b6_setup_utf8(&utf8, text, sizeof(text) - 1);
		b6_extend_utf8_string(&s, &utf8);
		b6_extend_utf8_string(&s, label);
	}
	while (s.utf8.nchars < 32)
		b6_append_utf8_string(&s, ' ');
	set_toolkit_label(&self->label[n], &s.utf8);
	b6_finalize_utf8_string(&s);
}

static void quit_phase(struct hall_of_fame_phase *self)
{
	self->quit = 1;
	set_fade_io_target(&self->fade_io, 0.);
}

static void on_key_pressed(struct controller_observer *observer,
			   enum controller_key key)
{
	struct hall_of_fame_phase *self = b6_cast_of(
		observer, struct hall_of_fame_phase, controller_observer);
	if (self->quit)
		return;
	if (!self->entry) {
		quit_phase(self);
		return;
	}
	if (key == CTRLK_RETURN || key == CTRLK_ENTER || key == CTRLK_ESCAPE) {
		quit_phase(self);
		return;
	}
	if (key != CTRLK_DELETE && key != CTRLK_BACKSPACE)
		return;
	if (b6_utf8_is_empty(&self->name.utf8))
		return;
	b6_assert(self->cursor_base);
	move_renderer_base(self->cursor_base, self->cursor_base->x - 16,
			   self->cursor_base->y);
	self->name.utf8.nchars -= 1; // FIXME: add crop API
	self->name.utf8.nbytes -= 1; // FIXME: add crop API
	alter_hall_of_fame_entry(self->entry, &self->name.utf8);
	setup_label(self, self->rank, self->entry);
}

static void on_text_input(struct controller_observer *observer,
			  unsigned short int unicode)
{
	struct hall_of_fame_phase *self = b6_cast_of(
		observer, struct hall_of_fame_phase, controller_observer);
	if (self->quit)
		return;
	if (!self->entry) {
		quit_phase(self);
		return;
	}
	if (unicode < 32)
		return;
	if (unicode > 127)
		return;
	if (self->name.utf8.nchars >= 16)
		return;
	b6_append_utf8_string(&self->name, unicode);
	b6_assert(self->cursor_base);
	move_renderer_base(self->cursor_base,
			   self->cursor_base->x + 16, self->cursor_base->y);
	alter_hall_of_fame_entry(self->entry, &self->name.utf8);
	setup_label(self, self->rank, self->entry);
}

static void on_render(struct renderer_observer *observer)
{
	struct hall_of_fame_phase *self = b6_cast_of(
		observer, struct hall_of_fame_phase, renderer_observer);
	if (!self->cursor_base)
		return;
	if (b6_get_clock_time(self->up.engine->clock) & 524288ULL)
		show_renderer_base(self->cursor_base);
	else
		hide_renderer_base(self->cursor_base);
}

static int hall_of_fame_phase_init(struct phase *up, const struct phase *prev)
{
	static const struct controller_observer_ops controller_observer_ops = {
		.on_key_pressed = on_key_pressed,
		.on_text_input = on_text_input,
	};
	static const struct renderer_observer_ops renderer_observer_ops = {
		.on_render = on_render,
	};
	struct hall_of_fame_phase *self =
		b6_cast_of(up, struct hall_of_fame_phase, up);
	struct renderer *renderer = get_engine_renderer(up->engine);
	struct renderer_base *root;
	unsigned short int font_w, font_h;
	struct hall_of_fame_iterator iter;
	struct data_entry *entry;
	struct istream *is;
	const char *skin_id = hof_skin ? hof_skin : get_skin_id();
	int i;
	self->array = open_hall_of_fame(&up->engine->hall_of_fame,
					up->engine->layout_provider->id,
					up->engine->game_config->entry.id);
	if (!self->array)
		return -1;
	if (b6_utf8_is_empty(&self->name.utf8)) {
		struct b6_utf8 utf8;
		if (user || (user = get_user_name()))
			b6_utf8_from_ascii(&utf8, user);
		else
			b6_clone_utf8(&utf8, B6_UTF8("PLAYER"));
		b6_clear_utf8_string(&self->name);
		b6_extend_utf8_string(&self->name, &utf8);
	}
	self->entry = NULL;
	self->quit = 0;
	if (prev == lookup_phase(B6_UTF8("game"))) {
		struct game_result game_result;
		get_last_game_result(up->engine, &game_result);
		self->entry = amend_hall_of_fame(self->array,
						 game_result.level + 1,
						 game_result.score);
		if (!self->entry)
			self->quit = 1;
		else
			alter_hall_of_fame_entry(self->entry, &self->name.utf8);
	}
	root = get_renderer_base(renderer);
	if (make_font(&self->font, skin_id, HOF_FONT_DATA_ID))
		return -1;
	font_w = get_fixed_font_width(&self->font);
	font_h = get_fixed_font_height(&self->font);

	if ((self->background = create_renderer_tile(renderer, root, 0, 0,
						     640, 480, NULL)))
		set_renderer_tile_texture(self->background, make_texture(
				renderer, skin_id, HOF_BACKGROUND_DATA_ID));
	if ((self->panel = create_renderer_tile(renderer, root, 0, 0, 112, 480,
						NULL)))
		set_renderer_tile_texture(self->panel, make_texture(
				renderer, skin_id, HOF_PANEL_DATA_ID));
	for (i = 0; i < b6_card_of(self->label); i += 1) {
		const unsigned short int u = 2 + 32 * font_w, v = 2 + font_h;
		const float x = 96, y = 50 + i * 40;
		const float w = 2 + 32 * 16, h = 2 + 16;
		initialize_toolkit_label(&self->label[i], renderer,
					 &self->font, u, v, root, x, y, w, h);
		enable_toolkit_label_shadow(&self->label[i]);
	}
	reset_hall_of_fame_iterator(&iter, self->array);
	for (i = 0; i < b6_card_of(self->label); i += 1) {
		struct b6_json_object *entry;
		if (!hall_of_fame_iterator_has_next(&iter))
			break;
		entry = hall_of_fame_iterator_get_next(&iter);
		if (entry == self->entry)
			self->rank = i;
		setup_label(self, i, entry);
	}
	for (; i < b6_card_of(self->label); i += 1)
		hide_toolkit_label(&self->label[i]);
	if (self->entry) {
		float x = self->label[self->rank].image[0].tile->x +
			(16 + self->name.utf8.nchars) * 16;
		float y = self->label[self->rank].image[0].tile->y;
		self->cursor_base = create_renderer_base_or_die(renderer, root,
								"cursor", x, y);
		initialize_toolkit_label(&self->cursor_label, renderer,
					 &self->font, 16, 16,
					 self->cursor_base, 1, 1, 16, 16);
		enable_toolkit_label_shadow(&self->cursor_label);
		set_toolkit_label(&self->cursor_label, &b6_utf8_char['#']);
	} else
		self->cursor_base = NULL;
	add_renderer_observer(renderer, setup_renderer_observer(
			&self->renderer_observer, "hof",
			&renderer_observer_ops));
	add_controller_observer(get_engine_controller(up->engine),
				setup_controller_observer(
					&self->controller_observer,
					&controller_observer_ops));
	self->music = 0;
	initialize_fade_io(&self->fade_io, "hof_fade_io", renderer,
			   up->engine->clock, 0., 1., 5e-6);
	if (self->quit) {
		set_fade_io_target(&self->fade_io, 0.);
		return 0;
	}
	if ((entry = lookup_data(skin_id, audio_data_type,
				 HOF_MUSIC_DATA_ID)) &&
	    (is = get_data(entry))) {
		int retval = up->engine->mixer->ops->load_music_from_stream(
			up->engine->mixer, is);
		if (!retval) {
			play_music(up->engine->mixer);
			self->music = 1;
		} else
			log_w(_s("could not load background music"));
		put_data(entry, is);
	} else
		log_w(_s("cannot find background music"));
	return 0;
}

static void hall_of_fame_phase_exit(struct phase *up)
{
	struct hall_of_fame_phase *self =
		b6_cast_of(up, struct hall_of_fame_phase, up);
	int i;
	finalize_fade_io(&self->fade_io);
	if (self->entry) {
		save_hall_of_fame(&up->engine->hall_of_fame);
		self->entry = NULL;
	}
	if (self->music) {
		stop_music(up->engine->mixer);
		unload_music(up->engine->mixer);
	}
	del_controller_observer(&self->controller_observer);
	del_renderer_observer(&self->renderer_observer);
	if (self->cursor_base) {
		finalize_toolkit_label(&self->cursor_label);
		destroy_renderer_base(self->cursor_base);
	}
	for (i = 0; i < b6_card_of(self->label); i += 1)
		finalize_toolkit_label(&self->label[i]);
	if (self->panel) {
		destroy_renderer_texture(self->panel->texture);
		destroy_renderer_tile(self->panel);
	}
	if (self->background) {
		destroy_renderer_texture(self->background->texture);
		destroy_renderer_tile(self->background);
	}
	finalize_fixed_font(&self->font);
	close_hall_of_fame(self->array);
}

static struct phase *hall_of_fame_phase_exec(struct phase *up)
{
	struct hall_of_fame_phase *self =
		b6_cast_of(up, struct hall_of_fame_phase, up);
	if (self->quit && fade_io_is_stopped(&self->fade_io))
		return lookup_phase(B6_UTF8("menu"));
	return up;
}

static int hall_of_fame_phase_ctor(void)
{
	static const struct phase_ops ops = {
		.init = hall_of_fame_phase_init,
		.exit = hall_of_fame_phase_exit,
		.exec = hall_of_fame_phase_exec,
	};
	static struct hall_of_fame_phase singleton;
	b6_reset_fixed_allocator(&singleton.fixed_allocator, singleton.buffer,
				 sizeof(singleton.buffer) - 1);
	b6_initialize_utf8_string(&singleton.name,
				  &singleton.fixed_allocator.allocator);
	return register_phase(&singleton.up, B6_UTF8("hall_of_fame"), &ops);
}
register_init(hall_of_fame_phase_ctor);
