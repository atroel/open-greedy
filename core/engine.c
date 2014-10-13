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

#include "console.h"
#include "data.h"
#include "engine.h"
#include "game.h"
#include "lang.h"
#include "level.h"
#include "renderer.h"

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

void setup_engine(struct engine *self, const struct b6_clock *clock,
		  struct console *console, struct mixer *mixer)
{
	self->clock = clock;
	self->console = console;
	self->mixer = mixer;
	self->lang = lookup_lang(lang);
	if (!(self->layout_provider = lookup_layout_provider(game))) {
		self->layout_provider = get_default_layout_provider();
		b6_check(self->layout_provider);
		log_w("Could not find %s level set. Falling back to %s.", game,
		      self->layout_provider->entry.name);
	}
	self->shuffle = shuffle;
	if (!(self->game_config = lookup_game_config(mode))) {
		log_w("unknown mode: %s", mode);
		self->game_config = get_default_game_config();
		log_w("falling back to: %s", self->game_config->entry.name);
	}
	self->game_result.score = 0ULL;
	self->game_result.level = 0ULL;
}

void reset_engine(struct engine *self)
{
	stop_renderer(self->console->default_renderer);
	close_console(self->console);
	open_console(self->console);
	start_renderer(self->console->default_renderer, 640, 480);
}

void run_engine(struct engine *self)
{
	struct phase *prev = NULL, *curr = lookup_phase("menu");
	open_console(self->console);
	while (curr) {
		struct phase *next = NULL;
		start_renderer(self->console->default_renderer, 640, 480);
		curr->engine = self;
		if (curr->ops->init && curr->ops->init(curr, self)) {
			curr = prev;
			continue;
		}
		do {
			poll_console(self->console);
			next = curr->ops->exec(curr, self);
			show_console(self->console);
		} while (next == curr);
		if (curr->ops->exit)
			curr->ops->exit(curr, self);
		curr->engine = NULL;
		stop_renderer(self->console->default_renderer);
		prev = curr;
		curr = next;
	}
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
