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

#include "hall_of_fame_phase.h"

#include "lib/init.h"

#include "console.h"
#include "controller.h"
#include "engine.h"
#include "fade_io.h"
#include "game.h"
#include "hall_of_fame.h"
#include "mixer.h"
#include "renderer.h"
#include "data.h"
#include "toolkit.h"

#include <b6/clock.h>
#include <b6/flags.h>

static const char *hof_skin = NULL;
b6_flag(hof_skin, string);

struct hall_of_fame_phase {
	struct phase up;
	struct engine *engine;
	struct hall_of_fame *hall_of_fame;
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
	struct hall_of_fame_entry *entry;
	char name[32];
	unsigned long int size;
	unsigned long int rank;
};

static const char *user = "PLAYER";
b6_flag(user, string);

static void u32_to_str(char *str, unsigned long int num, int len)
{
	while (len--) {
		str[len] = '0' + (num % 10);
		num /= 10;
	}
}

static void setup_label(int n, struct toolkit_label *label,
			unsigned long int level, unsigned long int score,
			const char *name)
{
	int i;
	const char *c;
	char text[] = "00 000 00000000 1234567890ABCDEF";
	u32_to_str(&text[0], n + 1, 2);
	u32_to_str(&text[3], level, 3);
	u32_to_str(&text[7], score, 8);
	for (i = 0, c = name; i < 16; i += 1)
		text[16 + i] = *c ? *c++ : ' ';
	set_toolkit_label(label, text);
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
	if (key == CTRLK_RETURN || key == CTRLK_ESCAPE) {
		quit_phase(self);
		return;
	}
	if (key != CTRLK_DELETE && key != CTRLK_BACKSPACE)
		return;
	if (self->size <= 0)
		return;
	self->size -= 1;
	b6_assert(self->cursor_base);
	move_renderer_base(self->cursor_base, self->cursor_base->x -
			   get_fixed_font_width(&self->font),
			   self->cursor_base->y);
	self->name[self->size] = '\0';
	setup_label(self->rank, &self->label[self->rank],
		    self->entry->level, self->entry->score, self->name);
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
	if (unicode >= 'a' && unicode <= 'z')
		unicode -= ' ';
	if (unicode < 32)
		return;
	if (unicode > 95)
		return;
	if (self->size >= sizeof(self->entry->name))
		return;
	self->name[self->size++] = unicode;
	b6_assert(self->cursor_base);
	move_renderer_base(
		self->cursor_base,
		self->cursor_base->x + get_fixed_font_width(&self->font),
		self->cursor_base->y);
	self->name[self->size] = '\0';
	setup_label(self->rank, &self->label[self->rank], self->entry->level,
		    self->entry->score, self->name);
}

static void on_render(struct renderer_observer *observer)
{
	struct hall_of_fame_phase *self = b6_cast_of(
		observer, struct hall_of_fame_phase, renderer_observer);
	if (!self->cursor_base)
		return;
	if (b6_get_clock_time(self->engine->clock) & 524288ULL)
		show_renderer_base(self->cursor_base);
	else
		hide_renderer_base(self->cursor_base);
}

static int hall_of_fame_phase_init(struct phase *up, struct engine *engine)
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
	struct renderer *renderer = get_engine_renderer(engine);
	struct renderer_base *root;
	unsigned short int font_w, font_h;
	struct hall_of_fame_iterator iter;
	struct data_entry *entry;
	struct istream *is;
	const char *skin_id = hof_skin ? hof_skin : get_skin_id();
	int i;
	self->hall_of_fame = load_hall_of_fame(engine->level_data_name,
					       engine->game_config->entry.name);
	self->engine = engine;
	self->entry = NULL;
	self->quit = 0;
	if (engine->game_result.level) {
		self->entry = get_hall_of_fame_entry(self->hall_of_fame,
						     engine->game_result.level,
						     engine->game_result.score);
		if (!self->entry)
			self->quit = 1;
		engine->game_result.level = 0UL;
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
	setup_hall_of_fame_iterator(self->hall_of_fame, &iter);
	for (i = 0; i < b6_card_of(self->label); i += 1) {
		struct hall_of_fame_entry *entry =
			hall_of_fame_iterator_next(&iter);
		if (!entry)
			break;
		if (entry != self->entry) {
			setup_label(i, &self->label[i], entry->level,
				    entry->score, entry->name);
			continue;
		}
		for (self->size = 0; self->size < sizeof(self->name);
		     self->size += 1)
			if (!(self->name[self->size] = user[self->size]))
				break;
		self->rank = i;
		setup_label(i, &self->label[i], entry->level, entry->score,
			    self->name);
	}
	for (; i < b6_card_of(self->label); i += 1)
		hide_toolkit_label(&self->label[i]);
	if (self->entry) {
		float x = self->label[self->rank].image[0].tile->x +
			(16 + self->size) * 16;
		float y = self->label[self->rank].image[0].tile->y;
		self->cursor_base = create_renderer_base_or_die(renderer, root,
								"cursor", x, y);
		initialize_toolkit_label(&self->cursor_label, renderer,
					 &self->font, font_w, font_h,
					 self->cursor_base, 1, 1, 16, 16);
		enable_toolkit_label_shadow(&self->cursor_label);
		set_toolkit_label(&self->cursor_label, "#");
	} else
		self->cursor_base = NULL;
	add_renderer_observer(renderer, setup_renderer_observer(
			&self->renderer_observer, "hof",
			&renderer_observer_ops));
	add_controller_observer(get_engine_controller(engine),
				setup_controller_observer(
					&self->controller_observer,
					&controller_observer_ops));
	self->music = 0;
	initialize_fade_io(&self->fade_io, "hof_fade_io", renderer,
			   engine->clock, 0., 1., 5e-6);
	if (self->quit) {
		set_fade_io_target(&self->fade_io, 0.);
		return 0;
	}
	if ((entry = lookup_data(skin_id, audio_data_type,
				 HOF_MUSIC_DATA_ID)) &&
	    (is = get_data(entry))) {
		int retval = engine->mixer->ops->load_music_from_stream(
			engine->mixer, is);
		if (!retval) {
			play_music(engine->mixer);
			self->music = 1;
		} else
			log_w("could not load background music");
		put_data(entry, is);
	} else
		log_w("cannot find background music");
	return 0;
}

static void hall_of_fame_phase_exit(struct phase *up, struct engine *engine)
{
	struct hall_of_fame_phase *self =
		b6_cast_of(up, struct hall_of_fame_phase, up);
	int i;
	finalize_fade_io(&self->fade_io);
	if (self->entry) {
		put_hall_of_fame_entry(self->hall_of_fame, self->entry,
				       self->name);
		save_hall_of_fame(self->hall_of_fame);
		self->entry = NULL;
	}
	engine->game_result.score = 0ULL;
	if (self->music) {
		stop_music(engine->mixer);
		unload_music(engine->mixer);
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
}

static struct phase *hall_of_fame_phase_exec(struct phase *up,
					     struct engine *engine)
{
	struct hall_of_fame_phase *self =
		b6_cast_of(up, struct hall_of_fame_phase, up);
	if (self->quit && fade_io_is_stopped(&self->fade_io))
		return lookup_phase("menu");
	return up;
}

static int hall_of_fame_phase_ctor(void)
{
	static const struct phase_ops ops = {
		.init = hall_of_fame_phase_init,
		.exit = hall_of_fame_phase_exit,
		.exec = hall_of_fame_phase_exec,
	};
	static struct hall_of_fame_phase hall_of_fame_phase;
	return register_phase(&hall_of_fame_phase.up, "hall_of_fame", &ops);
}
register_init(hall_of_fame_phase_ctor);
