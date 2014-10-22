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

#include <b6/clock.h>
#include <b6/cmdline.h>

#include "lib/init.h"

#include "console.h"
#include "controller.h"
#include "engine.h"
#include "game.h"
#include "lang.h"
#include "menu.h"
#include "menu_controller.h"
#include "menu_mixer.h"
#include "menu_renderer.h"
#include "renderer.h"
#include "data.h"

struct menu_phase;

struct submenu {
	const struct submenu_ops *ops;
};

struct submenu_ops {
	void (*enter)(struct submenu*, struct menu*);
	void (*update)(struct submenu*, struct engine*);
	struct submenu *(*select)(struct submenu*, struct menu_entry*,
				  struct menu_phase*);
};

struct main_menu {
	struct submenu up;
	struct menu_entry play;
	struct menu_entry options;
	struct menu_entry hof;
	struct menu_entry credits;
	struct menu_entry quit;
};

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
	struct submenu *submenu;
	struct phase *next;
};

static const char *menu_skin = NULL;
b6_flag(menu_skin, string);

static struct menu_phase *to_menu_phase(struct phase *up)
{
	return b6_cast_of(up, struct menu_phase, up);
}

static void enter_main(struct submenu *up, struct menu *menu)
{
	struct main_menu *self = b6_cast_of(up, struct main_menu, up);
	add_menu_entry(menu, &self->play);
	add_menu_entry(menu, &self->options);
	add_menu_entry(menu, &self->hof);
	add_menu_entry(menu, &self->credits);
	add_menu_entry(menu, &self->quit);
	set_quit_entry(menu, &self->quit);
}

static void update_main(struct submenu *up, struct engine *engine)
{
	struct main_menu *self = b6_cast_of(up, struct main_menu, up);
	self->play.text = engine->lang->menu.play;
	self->options.text = engine->lang->menu.options;
	self->hof.text = engine->lang->menu.hof;
	self->credits.text = engine->lang->menu.credits;
	self->quit.text = engine->lang->menu.quit;
}

static struct submenu *select_main(struct submenu *up, struct menu_entry *entry,
				   struct menu_phase *phase)
{
	struct main_menu *self = b6_cast_of(up, struct main_menu, up);
	if (entry == &self->options)
		return &phase->options_menu.up;
	else if (entry == &self->play)
		phase->next = lookup_phase("game");
	else if (entry == &self->quit)
		phase->next = NULL;
	else if (entry == &self->hof)
		phase->next = lookup_phase("hall_of_fame");
	else if (entry == &self->credits)
		phase->next = lookup_phase("credits");
	return up;
}

static void enter_options(struct submenu *up, struct menu *menu)
{
	struct options_menu *self = b6_cast_of(up, struct options_menu, up);
	add_menu_entry(menu, &self->game_options);
	add_menu_entry(menu, &self->video_options);
	add_menu_entry(menu, &self->lang);
	add_menu_entry(menu, &self->back);
	set_quit_entry(menu, &self->back);
}

static void update_options(struct submenu *up, struct engine *engine)
{
	struct options_menu *self = b6_cast_of(up, struct options_menu, up);
	self->game_options.text = engine->lang->menu.game_options;
	self->video_options.text = engine->lang->menu.video_options;
	self->lang.text = engine->lang->menu.lang;
	self->back.text = engine->lang->menu.back;
}

static struct submenu *select_options(struct submenu *up,
				      struct menu_entry *entry,
				      struct menu_phase *phase)
{
	struct options_menu *self = b6_cast_of(up, struct options_menu, up);
	if (entry == &self->game_options)
		return &phase->game_options_menu.up;
	if (entry == &self->video_options)
		return &phase->video_options_menu.up;
	if (entry == &self->back)
		return &phase->main_menu.up;
	if (entry == &self->lang)
		phase->up.engine->lang = get_next_lang(phase->up.engine->lang);
	return up;
}

static void enter_game_options(struct submenu *up, struct menu *menu)
{
	struct game_options_menu *self =
		b6_cast_of(up, struct game_options_menu, up);
	add_menu_entry(menu, &self->game);
	add_menu_entry(menu, &self->shuffle);
	add_menu_entry(menu, &self->mode);
	add_menu_entry(menu, &self->back);
	set_quit_entry(menu, &self->back);
}

static void update_game_options(struct submenu *up, struct engine *engine)
{
	static const struct game_config *slow = NULL;
	static const struct game_config *fast = NULL;
	struct game_options_menu *self =
		b6_cast_of(up, struct game_options_menu, up);
	const char *s = engine->lang->menu.game;
	char *d = self->game_text;
	char *e = d + b6_card_of(self->game_text) - 1;
	while (*s && d < e) *d++ = *s++;
	s = engine->layout_provider->entry.name;
	while (*s && d < e) *d++ = *s++;
	*d = '\0';
	self->game.text = self->game_text;
	if (!slow)
		slow = lookup_game_config("slow");
	if (!fast)
		fast = lookup_game_config("fast");
	if (engine->game_config == slow)
		self->mode.text = engine->lang->menu.mode_slow;
	else if (engine->game_config == fast)
		self->mode.text = engine->lang->menu.mode_fast;
	else
		self->mode.text = "MODE: <UNKNOWN>";
	self->shuffle.text = engine->shuffle ? engine->lang->menu.shuffle_on :
		engine->lang->menu.shuffle_off;
	self->back.text = engine->lang->menu.back;
}

static struct submenu *select_game_options(struct submenu *up,
					   struct menu_entry *entry,
					   struct menu_phase *phase)
{
	struct game_options_menu *self =
		b6_cast_of(up, struct game_options_menu, up);
	struct engine *engine = phase->up.engine;
	if (entry == &self->back)
		return &phase->options_menu.up;
	if (entry == &self->game)
		engine->layout_provider =
			get_next_layout_provider(engine->layout_provider);
	else if (entry == &self->mode)
		engine->game_config = get_next_game_config(engine->game_config);
	else if (entry == &self->shuffle)
		engine->shuffle = !engine->shuffle;
	return up;
}

static void enter_video_options(struct submenu *up, struct menu *menu)
{
	struct video_options_menu *self =
		b6_cast_of(up, struct video_options_menu, up);
	self->vs = get_console_vsync();
	self->fs = get_console_fullscreen();
	add_menu_entry(menu, &self->fullscreen);
	add_menu_entry(menu, &self->vsync);
	add_menu_entry(menu, &self->apply);
	add_menu_entry(menu, &self->back);
	set_quit_entry(menu, &self->back);
}

static void update_video_options(struct submenu *up, struct engine *engine)
{
	struct video_options_menu *self =
		b6_cast_of(up, struct video_options_menu, up);
	self->fullscreen.text = self->fs ?  engine->lang->menu.fullscreen_on :
		engine->lang->menu.fullscreen_off;
	self->vsync.text = self->vs ?  engine->lang->menu.vsync_on :
		engine->lang->menu.vsync_off;
	self->apply.text = engine->lang->menu.apply;
	self->back.text = engine->lang->menu.back;
}

static struct submenu *select_video_options(struct submenu *up,
					    struct menu_entry *entry,
					    struct menu_phase *phase)
{
	struct video_options_menu *self =
		b6_cast_of(up, struct video_options_menu, up);
	if (entry == &self->back)
		return &phase->options_menu.up;
	if (entry == &self->fullscreen)
		self->fs = !self->fs;
	else if (entry == &self->vsync)
		self->vs = !self->vs;
	else if (entry == &self->apply &&
		 (self->vs != get_console_vsync() ||
		  self->fs != get_console_fullscreen())) {
		struct engine *engine = phase->up.engine;
		set_console_vsync(self->vs);
		set_console_fullscreen(self->fs);
		finalize_menu_controller(&phase->controller);
		del_menu_observer(&phase->menu_observer);
		close_menu_renderer(&phase->renderer);
		finalize_menu_renderer(&phase->renderer);
		reset_engine(engine);
		initialize_menu_renderer(&phase->renderer,
					 get_engine_renderer(engine),
					 engine->clock, get_skin_id());
		open_menu_renderer(&phase->renderer, &phase->menu);
		add_menu_observer(&phase->menu, &phase->menu_observer);
		initialize_menu_controller(&phase->controller, &phase->menu,
					   get_engine_controller(engine));
	}
	return up;
}

static void leave_submenu(struct menu_phase *self)
{
	del_menu_observer(&self->menu_observer);
	close_menu_renderer(&self->renderer);
	self->submenu = NULL;
}

static void enter_submenu(struct menu_phase *self, struct submenu *submenu)
{
	initialize_menu(&self->menu);
	self->submenu = submenu;
	self->submenu->ops->enter(self->submenu, &self->menu);
	self->submenu->ops->update(self->submenu, self->up.engine);
	open_menu_renderer(&self->renderer, &self->menu);
	add_menu_observer(&self->menu, &self->menu_observer);
}

static void on_select(struct menu_observer *menu_observer)
{
	struct menu_phase *self =
		b6_cast_of(menu_observer, struct menu_phase, menu_observer);
	struct menu_entry *entry = get_current_menu_entry(&self->menu);
	struct submenu *submenu = self->submenu->ops->select(self->submenu,
							     entry, self);
	self->submenu->ops->update(self->submenu, self->up.engine);
	update_menu_renderer(&self->renderer);
	if (submenu == self->submenu)
		return;
	leave_submenu(self);
	enter_submenu(self, submenu);
}

static int menu_phase_init(struct phase *up)
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
	self->next = up;
	self->main_menu.up.ops = &main_ops;
	self->options_menu.up.ops = &options_ops;
	self->game_options_menu.up.ops = &game_options_ops;
	self->video_options_menu.up.ops = &video_options_ops;
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
