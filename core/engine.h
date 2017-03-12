/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014-2017 Arnaud TROEL
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

#include <b6/json.h>
#include <b6/registry.h>

#include "core/console.h"
#include "core/controller.h"
#include "core/hall_of_fame.h"
#include "core/level.h"

struct game_result {
	unsigned long int level; /* final level in the game */
	unsigned long int score; /* final score in the game */
};

struct preferences;

struct engine {
	const struct b6_clock *clock;
	struct console *console;
	struct mixer *mixer;
	const struct game_config *game_config;
	struct layout_provider *layout_provider;
	struct layout_shuffler layout_shuffler;
	int quit;
	struct phase *curr;
	struct game_result game_result;
	struct controller_observer observer;
	struct preferences *pref;
	struct b6_json_object *languages;
	struct b6_json_iterator iter;
	struct hall_of_fame hall_of_fame;
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

static inline const struct b6_json_pair *get_engine_language(
	struct engine *self)
{
	return b6_json_get_iterator(&self->iter);
}

extern void rotate_engine_language(struct engine *self);

extern void set_last_game_result(struct engine *self,
				 const struct game_result *result);

extern void get_last_game_result(struct engine *self,
				 struct game_result *result);

struct phase {
	struct b6_entry entry;
	const struct phase_ops *ops;
	struct engine *engine;
};

struct phase_ops {
	int (*init)(struct phase*, const struct phase*);
	void (*exit)(struct phase*);
	struct phase *(*exec)(struct phase*);
	void (*suspend)(struct phase*);
	void (*resume)(struct phase*);
};

extern int initialize_engine(struct engine *self, const struct b6_clock *clock,
			     struct console *console, struct mixer *mixer,
			     struct preferences *pref,
			     struct b6_json_object *languages);

extern void finalize_engine(struct engine *self);

extern void reset_engine(struct engine *self);

extern void run_engine(struct engine*);

extern struct b6_registry __phase_registry;

static inline int register_phase(struct phase *self, const struct b6_utf8 *id,
				 const struct phase_ops *ops)
{
	self->ops = ops;
	return b6_register(&__phase_registry, &self->entry, id);
}

static inline void unregister_phase(struct phase *self)
{
	return b6_unregister(&__phase_registry, &self->entry);
}

static inline struct phase *lookup_phase(const struct b6_utf8 *id)
{
	struct b6_entry *entry = b6_lookup_registry(&__phase_registry, id);
	return entry ? b6_cast_of(entry, struct phase, entry) : NULL;
}

#endif /* ENGINE_H */
