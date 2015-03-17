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

#include <b6/clock.h>
#include <b6/cmdline.h>
#include <b6/json.h>
#include <b6/utf8.h>

#include "lib/init.h"

#include "console.h"
#include "controller.h"
#include "data.h"
#include "engine.h"
#include "game.h"
#include "json.h"
#include "menu.h"
#include "menu_controller.h"
#include "menu_mixer.h"
#include "menu_renderer.h"
#include "preferences.h"
#include "renderer.h"

struct menu_phase;

struct submenu {
	const struct submenu_ops *ops;
	struct menu_phase *menu_phase;
	struct engine *engine;
};

struct submenu_ops {
	void (*enter)(struct submenu*, const struct b6_json_object*);
	void (*update)(struct submenu*, const struct b6_json_object*);
	struct submenu *(*select)(struct submenu*, struct menu_entry*);
	void (*leave)(struct submenu*);
};

struct main_menu {
	struct submenu up;
	struct menu_entry play;
	struct menu_entry resume;
	struct menu_entry options;
	struct menu_entry hof;
	struct menu_entry credits;
	struct menu_entry quit;
};

static int setup_submenu(struct submenu *self, const struct submenu_ops *ops,
			 struct menu_phase *menu_phase, struct engine *engine)
{
	self->ops = ops;
	self->menu_phase = menu_phase;
	self->engine = engine;
	return 0;
}

struct options_menu {
	struct submenu up;
	struct menu_entry game_options;
	struct menu_entry video_options;
	struct menu_entry lang;
	struct menu_entry back;
};

struct game_options_menu {
	struct submenu up;
	char game_text[32];
	struct menu_entry game;
	struct menu_entry mode;
	struct menu_entry shuffle;
	struct menu_entry back;
	struct b6_json_string *game_label;
};

struct video_options_menu {
	struct submenu up;
	int fs;
	int vs;
	struct menu_entry fullscreen;
	struct menu_entry vsync;
	struct menu_entry apply;
	struct menu_entry back;
};

struct menu_phase {
	struct phase up;
	struct menu menu;
	struct menu_observer menu_observer;
	struct menu_mixer mixer;
	struct menu_renderer renderer;
	struct menu_controller controller;
	struct main_menu main_menu;
	struct options_menu options_menu;
	struct game_options_menu game_options_menu;
	struct video_options_menu video_options_menu;
	struct b6_json_object *lang;
	struct submenu *submenu;
	struct phase *next;
};

static const char *menu_skin = NULL;
b6_flag(menu_skin, string);

static struct menu_phase *to_menu_phase(struct phase *up)
{
	return b6_cast_of(up, struct menu_phase, up);
}

static void enter_main(struct submenu *up, const struct b6_json_object *lang)
{
	struct main_menu *self = b6_cast_of(up, struct main_menu, up);
	struct game_result game_result;
	get_last_game_result(up->engine, &game_result);
	if (game_result.level)
		add_menu_entry(&up->menu_phase->menu, &self->resume);
	add_menu_entry(&up->menu_phase->menu, &self->play);
	add_menu_entry(&up->menu_phase->menu, &self->options);
	add_menu_entry(&up->menu_phase->menu, &self->hof);
	add_menu_entry(&up->menu_phase->menu, &self->credits);
	add_menu_entry(&up->menu_phase->menu, &self->quit);
	set_quit_entry(&up->menu_phase->menu, &self->quit);
}

static void setup_menu_entry_utf8(struct menu_entry *entry,
				  const struct b6_json_object *lang,
				  const void *utf8, unsigned int size)
{
	const struct b6_json_string *text =
		b6_json_get_object_utf8_as(lang, utf8, size, string);
	if (!text) {
		log_w("could not find menu text entry");
		entry->utf8_data = utf8;
		entry->utf8_size = size;
	} else {
		entry->utf8_data = b6_json_string_utf8(text);
		entry->utf8_size = b6_json_string_size(text);
	}
}

static void setup_menu_entry(struct menu_entry *entry,
			     const struct b6_json_object *lang,
			     const char *key)
{
	const struct b6_json_string *text =
		b6_json_get_object_as(lang, key, string);
	if (!text) {
		log_w("could not find menu text entry \"%s\"", key);
		entry->utf8_data = b6_ascii_to_utf8(key, &entry->utf8_size);
	} else {
		entry->utf8_data = b6_json_string_utf8(text);
		entry->utf8_size = b6_json_string_size(text);
	}
}

static void update_main(struct submenu *up, const struct b6_json_object *lang)
{
	struct main_menu *self = b6_cast_of(up, struct main_menu, up);
	setup_menu_entry(&self->resume, lang, "resume");
	setup_menu_entry(&self->play, lang, "play");
	setup_menu_entry(&self->options, lang, "options");
	setup_menu_entry(&self->hof, lang, "hof");
	setup_menu_entry(&self->credits, lang, "credits");
	setup_menu_entry(&self->quit, lang, "quit");
}

static struct submenu *select_main(struct submenu *up, struct menu_entry *entry)
{
	struct main_menu *self = b6_cast_of(up, struct main_menu, up);
	struct menu_phase *phase = up->menu_phase;
	if (entry == &self->options)
		return &up->menu_phase->options_menu.up;
	else if (entry == &self->resume)
		phase->next = lookup_phase("game");
	else if (entry == &self->play) {
		struct game_result game_result = { .score = 0, .level = 0, };
		set_last_game_result(up->engine, &game_result);
		phase->next = lookup_phase("game");
	} else if (entry == &self->quit)
		phase->next = NULL;
	else if (entry == &self->hof)
		phase->next = lookup_phase("hall_of_fame");
	else if (entry == &self->credits)
		phase->next = lookup_phase("credits");
	return up;
}

static void enter_options(struct submenu *up, const struct b6_json_object *lang)
{
	struct options_menu *self = b6_cast_of(up, struct options_menu, up);
	struct menu_phase *phase = up->menu_phase;
	add_menu_entry(&phase->menu, &self->game_options);
	add_menu_entry(&phase->menu, &self->video_options);
	add_menu_entry(&phase->menu, &self->lang);
	add_menu_entry(&phase->menu, &self->back);
	set_quit_entry(&phase->menu, &self->back);
}

static void update_options(struct submenu *up,
			   const struct b6_json_object *lang)
{
	struct options_menu *self = b6_cast_of(up, struct options_menu, up);
	setup_menu_entry(&self->game_options, lang, "game_options");
	setup_menu_entry(&self->video_options, lang, "video_options");
	setup_menu_entry(&self->lang, lang, "lang");
	setup_menu_entry(&self->back, lang, "back");
}

static struct submenu *select_options(struct submenu *up,
				      struct menu_entry *entry)
{
	struct options_menu *self = b6_cast_of(up, struct options_menu, up);
	struct menu_phase *phase = up->menu_phase;
	if (entry == &self->game_options)
		return &phase->game_options_menu.up;
	if (entry == &self->video_options)
		return &phase->video_options_menu.up;
	if (entry == &self->back)
		return &phase->main_menu.up;
	if (entry == &self->lang) do {
		const struct b6_json_pair *pair;
		struct b6_json_object *lang;
		rotate_engine_language(phase->up.engine);
		pair = get_engine_language(phase->up.engine);
		set_pref_lang(up->engine->pref, b6_json_string_utf8(pair->key),
			      b6_json_string_size(pair->key));
		lang = b6_json_value_as(get_engine_language(up->engine)->value,
					object);
		phase->lang = b6_json_get_object_as(lang, "menu", object);
	} while (!phase->lang);
	return up;
}

static void enter_game_options(struct submenu *up,
			       const struct b6_json_object *lang)
{
	struct game_options_menu *self =
		b6_cast_of(up, struct game_options_menu, up);
	struct menu_phase *phase = up->menu_phase;
	add_menu_entry(&phase->menu, &self->game);
	add_menu_entry(&phase->menu, &self->shuffle);
	add_menu_entry(&phase->menu, &self->mode);
	add_menu_entry(&phase->menu, &self->back);
	set_quit_entry(&phase->menu, &self->back);
	self->game_label = NULL;
}

static void update_game_options(struct submenu *up,
				const struct b6_json_object *lang)
{
	struct game_options_menu *self =
		b6_cast_of(up, struct game_options_menu, up);
	struct b6_json_string *text;
	const void *utf8_data;
	unsigned int utf8_size;
	utf8_data = b6_ascii_to_utf8(up->engine->layout_provider->entry.name,
				     &utf8_size);
	set_pref_game(up->engine->pref, utf8_data, utf8_size);
	if (self->game_label)
		b6_json_unref_value(&self->game_label->up);
	self->game_label = b6_json_new_string(lang->json, NULL);
	if ((text = b6_json_get_object_as(lang, "game", string))) {
		self->game_label->impl->ops->append(self->game_label->impl,
						    self->game_label->json->impl,
						    b6_json_string_utf8(text),
						    b6_json_string_size(text));
	}
	self->game_label->impl->ops->append(
		self->game_label->impl, self->game_label->json->impl,
		utf8_data, utf8_size);
	self->game.utf8_data = b6_json_string_utf8(self->game_label);
	self->game.utf8_size = b6_json_string_size(self->game_label);
	setup_menu_entry_utf8(&self->mode, lang,
			      up->engine->game_config->entry.name,
			      up->engine->game_config->entry.size);
	setup_menu_entry(&self->shuffle, lang,
			 get_pref_shuffle(up->engine->pref) ?
			 "shuffle_on" : "shuffle_off");
	setup_menu_entry(&self->back, lang, "back");
}

static struct submenu *select_game_options(struct submenu *up,
					   struct menu_entry *entry)
{
	struct game_options_menu *self =
		b6_cast_of(up, struct game_options_menu, up);
	struct menu_phase *phase = up->menu_phase;
	if (entry == &self->back)
		return &phase->options_menu.up;
	if (entry == &self->game)
		up->engine->layout_provider =
			get_next_layout_provider(up->engine->layout_provider);
	else if (entry == &self->mode) {
		up->engine->game_config =
			get_next_game_config(up->engine->game_config);
		set_pref_mode(up->engine->pref,
			      up->engine->game_config->entry.name,
			      up->engine->game_config->entry.size);
	} else if (entry == &self->shuffle)
		set_pref_shuffle(up->engine->pref,
				 !get_pref_shuffle(up->engine->pref));
	return up;
}

static void leave_game_options(struct submenu *up)
{
	struct game_options_menu *self =
		b6_cast_of(up, struct game_options_menu, up);
	if (self->game_label)
		b6_json_unref_value(&self->game_label->up);
}

static void enter_video_options(struct submenu *up,
				const struct b6_json_object *lang)
{
	struct video_options_menu *self =
		b6_cast_of(up, struct video_options_menu, up);
	struct menu_phase *phase = up->menu_phase;
	self->vs = get_console_vsync();
	self->fs = get_console_fullscreen();
	add_menu_entry(&phase->menu, &self->fullscreen);
	add_menu_entry(&phase->menu, &self->vsync);
	add_menu_entry(&phase->menu, &self->apply);
	add_menu_entry(&phase->menu, &self->back);
	set_quit_entry(&phase->menu, &self->back);
}

static void update_video_options(struct submenu *up,
				 const struct b6_json_object *lang)
{
	struct video_options_menu *self =
		b6_cast_of(up, struct video_options_menu, up);
	setup_menu_entry(&self->fullscreen, lang,
			 self->fs ?  "fullscreen_on" : "fullscreen_off");
	setup_menu_entry(&self->vsync, lang,
			 self->vs ?  "vsync_on" : "vsync_off");
	setup_menu_entry(&self->apply, lang, "apply");
	setup_menu_entry(&self->back, lang, "back");
}

static struct submenu *select_video_options(struct submenu *up,
					    struct menu_entry *entry)
{
	struct video_options_menu *self =
		b6_cast_of(up, struct video_options_menu, up);
	struct menu_phase *phase = up->menu_phase;
	if (entry == &self->back)
		return &phase->options_menu.up;
	if (entry == &self->fullscreen) {
		self->fs = !self->fs;
		return up;
	}
	if (entry == &self->vsync) {
		self->vs = !self->vs;
		return up;
	}
	if (entry == &self->apply) {
		set_pref_vsync(up->engine->pref, self->vs);
		set_pref_fullscreen(up->engine->pref, self->fs);
		if (self->vs == get_console_vsync() &&
		    self->fs == get_console_fullscreen())
			return up;
		set_console_vsync(self->vs);
		set_console_fullscreen(self->fs);
		finalize_menu_controller(&phase->controller);
		del_menu_observer(&phase->menu_observer);
		close_menu_renderer(&phase->renderer);
		finalize_menu_renderer(&phase->renderer);
		reset_engine(up->engine);
		initialize_menu_renderer(&phase->renderer,
					 get_engine_renderer(up->engine),
					 up->engine->clock, get_skin_id());
		open_menu_renderer(&phase->renderer, &phase->menu);
		add_menu_observer(&phase->menu, &phase->menu_observer);
		initialize_menu_controller(&phase->controller, &phase->menu,
					   get_engine_controller(up->engine));
	}
	return up;
}

static void leave_submenu(struct menu_phase *self)
{
	if (self->submenu->ops->leave)
		self->submenu->ops->leave(self->submenu);
	del_menu_observer(&self->menu_observer);
	close_menu_renderer(&self->renderer);
	self->submenu = NULL;
}

static void enter_submenu(struct menu_phase *self, struct submenu *submenu)
{
	initialize_menu(&self->menu);
	self->submenu = submenu;
	self->submenu->ops->enter(self->submenu, self->lang);
	self->submenu->ops->update(self->submenu, self->lang);
	open_menu_renderer(&self->renderer, &self->menu);
	add_menu_observer(&self->menu, &self->menu_observer);
}

static void on_select(struct menu_observer *menu_observer)
{
	struct menu_phase *self =
		b6_cast_of(menu_observer, struct menu_phase, menu_observer);
	struct menu_entry *entry = get_current_menu_entry(&self->menu);
	struct submenu *submenu =
		self->submenu->ops->select(self->submenu, entry);
	self->submenu->ops->update(self->submenu, self->lang);
	update_menu_renderer(&self->renderer);
	if (submenu == self->submenu)
		return;
	leave_submenu(self);
	enter_submenu(self, submenu);
}

static int menu_phase_init(struct phase *up, const struct phase *prev)
{
	static const struct menu_observer_ops menu_observer_ops = {
		.on_select = on_select,
	};
	static const struct submenu_ops main_ops = {
		.enter = enter_main,
		.update = update_main,
		.select = select_main,
	};
	static const struct submenu_ops options_ops = {
		.enter = enter_options,
		.update = update_options,
		.select = select_options,
	};
	static const struct submenu_ops game_options_ops = {
		.enter = enter_game_options,
		.leave = leave_game_options,
		.update = update_game_options,
		.select = select_game_options,
	};
	static const struct submenu_ops video_options_ops = {
		.enter = enter_video_options,
		.update = update_video_options,
		.select = select_video_options,
	};
	struct menu_phase *self = to_menu_phase(up);
	const char *skin_id = menu_skin ? menu_skin : get_skin_id();
	int retval;
	struct b6_json_object *lang;
	lang = b6_json_value_as(get_engine_language(up->engine)->value, object);
	if (!(self->lang = b6_json_get_object_as(lang, "menu", object))) {
		log_e("cannot find menu json object");
		return -1;
	}
	self->next = up;
	setup_submenu(&self->main_menu.up, &main_ops, self, up->engine);
	setup_submenu(&self->options_menu.up, &options_ops, self, up->engine);
	setup_submenu(&self->game_options_menu.up, &game_options_ops, self,
		      up->engine);
	setup_submenu(&self->video_options_menu.up, &video_options_ops, self,
		      up->engine);
	retval = initialize_menu_renderer(&self->renderer,
					  get_engine_renderer(up->engine),
					  up->engine->clock, skin_id);
	if (retval) {
		log_e("cannot initialize menu renderer (%d)", retval);
		return -1;
	}
	setup_menu_observer(&self->menu_observer, &menu_observer_ops);
	enter_submenu(self, &self->main_menu.up);
	initialize_menu_mixer(&self->mixer, &self->menu, skin_id,
			      up->engine->mixer);
	initialize_menu_controller(&self->controller, &self->menu,
				   get_engine_controller(up->engine));
	return 0;
}

static void menu_phase_exit(struct phase *up)
{
	struct menu_phase *self = to_menu_phase(up);
	finalize_menu_controller(&self->controller);
	finalize_menu_mixer(&self->mixer);
	leave_submenu(self);
	finalize_menu_renderer(&self->renderer);
}

static struct phase *menu_phase_exec(struct phase *up)
{
	return to_menu_phase(up)->next;
}

static int menu_phase_ctor(void)
{
	static const struct phase_ops ops = {
		.init = menu_phase_init,
		.exit = menu_phase_exit,
		.exec = menu_phase_exec,
	};
	static struct menu_phase menu_phase;
	return register_phase(&menu_phase.up, "menu", &ops);
}
register_init(menu_phase_ctor);
