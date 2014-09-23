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

#ifndef LANG_H
#define LANG_H

#include <b6/registry.h>

#include "lib/init.h"

struct lang {
	struct b6_entry entry;
	struct {
		const char *play;
		const char *hof;
		const char *credits;
		const char *quit;
		const char *mode_slow;
		const char *mode_fast;
	} menu;
	struct {
		const char *get_ready;
		const char *paused;
		const char *try_again;
		const char *game_over;
		const char *happy_hour;
		const char *bullet_proof;
		const char *freeze;
		const char *eat_the_ghosts;
		const char *extra_life;
		const char *empty_shield;
		const char *shield_pickup;
		const char *level_complete;
		const char *next_level;
		const char *previous_level;
		const char *slow_ghosts;
		const char *fast_ghosts;
		const char *speed_slow;
		const char *speed_fast;
		const char *single_booster;
		const char *double_booster;
		const char *booster_full;
		const char *booster_empty;
		const char *wiped_out;
		const char *bonus_200;
		const char *dieting;
		const char *casino;
	} game;
	struct {
		const char *lines[30];
	} credits;
};

#define register_lang(_self) \
	static struct lang _self; \
	static int register_lang_ ## _self(void) \
	{ \
		return b6_register(&__lang_registry, &(_self).entry, #_self); \
	} \
	register_init(register_lang_ ## _self); \
	static struct lang _self

extern const struct lang __lang_default;
extern struct b6_registry __lang_registry;

static inline const struct lang *lookup_lang(const char *name)
{
	const struct b6_entry *e = b6_lookup_registry(&__lang_registry, name);
	return e ? b6_cast_of(e, const struct lang, entry) : &__lang_default;
}

#endif /* LANG_H */
