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

#include "console.h"
#include "data.h"
#include "engine.h"
#include "game.h"
#include "json.h"
#include "level.h"
#include "mixer.h"
#include "renderer.h"

#include "lib/embedded.h"
#include <b6/cmdline.h>

static const char *game = "Greedy XP";
b6_flag(game, string);

static int shuffle = 0;
b6_flag(shuffle, bool);

static const char *mode = "fast";
b6_flag(mode, string);

const char *lang = "en";
b6_flag(lang, string);

B6_REGISTRY_DEFINE(__phase_registry);

static void on_quit(struct controller_observer *observer)
{
	b6_cast_of(observer, struct engine, observer)->quit = 1;
}

static void on_focus_in(struct controller_observer *observer)
{
	struct engine *self = b6_cast_of(observer, struct engine, observer);
	resume_mixer(self->mixer);
	if (self->curr && self->curr->ops->resume)
		self->curr->ops->resume(self->curr);
}

static void on_focus_out(struct controller_observer *observer)
{
	struct engine *self = b6_cast_of(observer, struct engine, observer);
	if (self->curr && self->curr->ops->suspend)
		self->curr->ops->suspend(self->curr);
	suspend_mixer(self->mixer);
}

const struct b6_json_object *rotate_engine_language(struct engine *self)
{
	const struct b6_json_pair *pair;
	b6_json_advance_iterator(&self->iter);
	if (!b6_json_get_iterator(&self->iter))
		b6_json_setup_iterator(&self->iter, self->languages);
	while ((pair = b6_json_get_iterator(&self->iter))) {
		if (b6_json_value_is_of(pair->value, object))
			break;
		b6_json_advance_iterator(&self->iter);
	}
	return get_engine_language(self);
}

static int setup_engine_language(struct engine *self,
				 struct b6_json_object *languages)
{
	const struct b6_json_pair *pair;
	self->languages = languages;
	b6_json_setup_iterator_at(&self->iter, languages, lang);
	pair = b6_json_get_iterator(&self->iter);
	if (pair && b6_json_value_is_of(pair->value, object))
		return 0;
	log_w("cannot find \"%s\" language", lang);
	b6_json_setup_iterator_at(&self->iter, languages, "en");
	pair = b6_json_get_iterator(&self->iter);
	if (pair && b6_json_value_is_of(pair->value, object)) {
		log_i("falling back to \"en\" language", lang);
		return 0;
	}
	log_w("cannot find fallback language");
	b6_json_setup_iterator(&self->iter, languages);
	while ((pair = b6_json_get_iterator(&self->iter))) {
		if (b6_json_value_is_of(pair->value, object))
			return 0;
		b6_json_advance_iterator(&self->iter);
	}
	return -1;
}

int setup_engine(struct engine *self, const struct b6_clock *clock,
		 struct console *console, struct mixer *mixer,
		 struct b6_json_object *languages)
{
	static const struct controller_observer_ops ops = {
		.on_quit = on_quit,
		.on_focus_in = on_focus_in,
		.on_focus_out = on_focus_out,
	};
	int retval;
	setup_controller_observer(&self->observer, &ops);
	self->clock = clock;
	self->console = console;
	self->mixer = mixer;
	if ((retval = setup_engine_language(self, languages)))
		return retval;
	if (!(self->layout_provider = lookup_layout_provider(game))) {
		self->layout_provider = get_default_layout_provider();
		b6_check(self->layout_provider);
		log_w("Could not find %s level set. Falling back to %s.", game,
		      self->layout_provider->entry.name);
	}
	self->shuffle = shuffle;
	self->quit = 0;
	if (!(self->game_config = lookup_game_config(mode))) {
		log_w("unknown mode: %s", mode);
		self->game_config = get_default_game_config();
		log_w("falling back to: %s", self->game_config->entry.name);
	}
	self->game_result.score = 0ULL;
	self->game_result.level = 0ULL;
	self->curr = lookup_phase("menu");
	return 0;
}

void reset_engine(struct engine *self)
{
	stop_renderer(self->console->default_renderer);
	del_controller_observer(&self->observer);
	close_console(self->console);
	open_console(self->console);
	add_controller_observer(self->console->default_controller,
				&self->observer);
	start_renderer(self->console->default_renderer, 640, 480);
}

static int init_phase(struct phase *self)
{
	return self->ops->init ? self->ops->init(self) : 0;
}

static struct phase *exec_phase(struct phase *self)
{
	b6_check(self->ops->exec);
	return self->ops->exec(self);
}

static void exit_phase(struct phase *self)
{
	if (self->ops->exit)
		self->ops->exit(self);
}

void run_engine(struct engine *self)
{
	struct phase *prev = NULL;
	open_console(self->console);
	add_controller_observer(self->console->default_controller,
				&self->observer);
	while (self->curr) {
		struct phase *next = self->curr;
		start_renderer(self->console->default_renderer, 640, 480);
		self->curr->engine = self;
		if (init_phase(self->curr)) {
			self->curr = prev;
			continue;
		}
		do {
			poll_console(self->console);
			next = exec_phase(self->curr);
			show_console(self->console);
			if (b6_unlikely(self->quit))
				next = NULL;
		} while (next == self->curr);
		exit_phase(self->curr);
		self->curr->engine = NULL;
		stop_renderer(self->console->default_renderer);
		prev = self->curr;
		self->curr = next;
	}
	del_controller_observer(&self->observer);
	close_console(self->console);
}

struct layout_provider *get_engine_layouts(const struct engine *self)
{
	struct layout_provider *layout_provider = self->layout_provider;
	if (self->shuffle) {
		struct engine *mutable = (struct engine*)self;
		reset_layout_shuffler(&mutable->layout_shuffler,
				      layout_provider);
		layout_provider = &mutable->layout_shuffler.up;
	}
	return layout_provider;
}
