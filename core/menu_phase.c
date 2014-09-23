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
#include <b6/flags.h>

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

struct menu_phase {
	struct phase up;
	struct menu menu;
	struct menu_observer menu_observer;
	struct menu_entry play;
	struct menu_entry mode;
	struct menu_entry hof;
	struct menu_entry credits;
	struct menu_entry quit;
	struct menu_mixer mixer;
	struct menu_renderer renderer;
	struct menu_controller controller;
	struct menu_skin *skin;
	struct phase *next;
};

static const char *menu_skin = NULL;
b6_flag(menu_skin, string);

static const struct game_config *slow = NULL;
static const struct game_config *fast = NULL;

static const struct game_config *get_next_mode(const struct game_config *config)
{
	if (!slow)
		slow = lookup_game_config("slow");
	if (!fast)
		fast = lookup_game_config("fast");
	if (config == slow)
		return fast;
	if (config == fast)
		return slow;
	return config;
}

static const char *game_config_to_menu_label(struct menu_phase *self,
					     const struct game_config *config)
{
	struct engine *engine = self->up.engine;
	if (!slow)
		slow = lookup_game_config("slow");
	if (!fast)
		fast = lookup_game_config("fast");
	if (config == slow)
		return engine->lang->menu.mode_slow;
	else if (config == fast)
		return engine->lang->menu.mode_fast;
	return "MODE: <UNKNOWN>";
}

static struct menu_phase *to_menu_phase(struct phase *up)
{
	return b6_cast_of(up, struct menu_phase, up);
}

static void on_select(struct menu_observer *menu_observer)
{
	struct menu_phase *self =
		b6_cast_of(menu_observer, struct menu_phase, menu_observer);
	struct menu_entry *entry = get_current_menu_entry(&self->menu);
	if (entry == &self->quit)
		self->next = NULL;
	else if (entry == &self->hof)
		self->next = lookup_phase("hall_of_fame");
	else if (entry == &self->credits)
		self->next = lookup_phase("credits");
	else if (entry == &self->mode) {
		struct engine *engine = self->up.engine;
		engine->game_config = get_next_mode(engine->game_config);
		self->mode.text =
			game_config_to_menu_label(self, engine->game_config);
		update_menu_renderer(&self->renderer);
	} else if (entry == &self->play)
		self->next = lookup_phase("game");
}

static int menu_phase_init(struct phase *up, struct engine *engine)
{
	static const struct menu_observer_ops menu_observer_ops = {
		.on_select = on_select,
	};
	struct menu_phase *self = to_menu_phase(up);
	const char *skin_id = menu_skin ? menu_skin : get_skin_id();
	int retval;
	self->next = up;
	initialize_menu(&self->menu);
	self->play.text = engine->lang->menu.play;
	add_menu_entry(&self->menu, &self->play);
	self->mode.text =
		game_config_to_menu_label(self, up->engine->game_config);
	add_menu_entry(&self->menu, &self->mode);
	self->hof.text = engine->lang->menu.hof;
	add_menu_entry(&self->menu, &self->hof);
	self->credits.text = engine->lang->menu.credits;
	add_menu_entry(&self->menu, &self->credits);
	self->quit.text = engine->lang->menu.quit;
	add_menu_entry(&self->menu, &self->quit);
	set_quit_entry(&self->menu, &self->quit);
	retval = initialize_menu_renderer(
		&self->renderer, get_engine_renderer(engine), engine->clock,
		&self->menu, skin_id,
		game_config_to_menu_label(self, engine->game_config),
		engine->lang);
	if (retval) {
		log_e("cannot initialize menu renderer (%d)", retval);
		return -1;
	}
	initialize_menu_mixer(&self->mixer, &self->menu, skin_id,
			      engine->mixer);
	initialize_menu_controller(&self->controller, &self->menu,
				   get_engine_controller(engine));
	setup_menu_observer(&self->menu_observer, &menu_observer_ops);
	add_menu_observer(&self->menu, &self->menu_observer);
	return 0;
}

static void menu_phase_exit(struct phase *up, struct engine *engine)
{
	struct menu_phase *self = to_menu_phase(up);
	del_menu_observer(&self->menu_observer);
	finalize_menu_mixer(&self->mixer);
	finalize_menu_renderer(&self->renderer);
	finalize_menu_controller(&self->controller);
}

static struct phase *menu_phase_exec(struct phase *up, struct engine *engine)
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
