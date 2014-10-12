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
#include "level.h"
#include "renderer.h"

#include <b6/cmdline.h>

B6_REGISTRY_DEFINE(__phase_registry);

static const char *fallback = "default";

static const char *probe_level_set(const char *level_data_name)
{
	struct data_entry *entry;
	struct istream *istream;
	if ((entry = lookup_data(level_data_name, level_data_type, "01")) &&
	    (istream = get_data(entry))) {
		struct layout layout;
		int retval = unserialize_layout(&layout, istream);
		put_data(entry, istream);
		if (!retval)
			return level_data_name;
	}
	log_e("level #01 of level set %s cannot be found or read. Falling back"
	      "to %s level set.", level_data_name, fallback);
	return fallback;
}

void setup_engine(struct engine *self, const struct b6_clock *clock,
		  struct console *console, struct mixer *mixer,
		  const struct lang *lang, const char* level_data_name,
		  const struct game_config *game_config)
{
	self->clock = clock;
	self->console = console;
	self->mixer = mixer;
	self->lang = lang;
	self->level_data_name = probe_level_set(level_data_name);
	self->game_config = game_config;
	self->game_result.score = 0ULL;
	self->game_result.level = 0ULL;
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
