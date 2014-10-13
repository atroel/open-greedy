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

#ifndef ENGINE_H
#define ENGINE_H

#include <b6/registry.h>
#include "core/console.h"
#include "core/level.h"

struct game_result {
	unsigned long int level; /* final level in the game */
	unsigned long int score; /* final score in the game */
};

struct engine {
	const struct b6_clock *clock;
	struct console *console;
	struct mixer *mixer;
	const struct lang *lang;
	const struct game_config *game_config;
	struct layout_provider *layout_provider;
	struct layout_shuffler layout_shuffler;
	int shuffle;
	struct game_result game_result;
};

static inline struct controller *get_engine_controller(const struct engine *e)
{
	return e->console->default_controller;
}

static inline struct renderer *get_engine_renderer(const struct engine *e)
{
	return e->console->default_renderer;
}

extern struct layout_provider *get_engine_layouts(const struct engine *e);

struct phase {
	struct b6_entry entry;
	const struct phase_ops *ops;
	struct engine *engine;
};

struct phase_ops {
	int (*init)(struct phase*, struct engine*);
	void (*exit)(struct phase*, struct engine*);
	struct phase *(*exec)(struct phase*, struct engine*);
};

extern void setup_engine(struct engine *self, const struct b6_clock *clock,
			 struct console *console, struct mixer *mixer,
			 const struct lang *lang,
			 const struct game_config *game_config);

extern void run_engine(struct engine*);

extern struct b6_registry __phase_registry;

static inline int register_phase(struct phase *self, const char *name,
				 const struct phase_ops *ops)
{
	self->ops = ops;
	return b6_register(&__phase_registry, &self->entry, name);
}

static inline void unregister_phase(struct phase *self)
{
	return b6_unregister(&__phase_registry, &self->entry);
}

static inline struct phase *lookup_phase(const char *name)
{
	struct b6_entry *entry = b6_lookup_registry(&__phase_registry, name);
	return entry ? b6_cast_of(entry, struct phase, entry) : NULL;
}

#endif /* ENGINE_H */
