/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014-2016 Arnaud TROEL
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

#ifndef GAME_H
#define GAME_H

#include "level.h"
#include "items.h"
#include "pacman.h"
#include "ghosts.h"
#include <b6/clock.h>
#include <b6/deque.h>
#include <b6/event.h>
#include <b6/list.h>
#include <b6/observer.h>
#include <b6/registry.h>

struct game_event {
	struct b6_event event;
	const char *name;
	struct game *game;
};

/* Counting from 0, levels 17, 31, 32, 34, 57.
 *
 * 't.' is a destination-only teleport.
 * '..' levels: 17, 31
 *
 * '@ g@'
 * '@ t@'
 * '@@@@' levels: 32, 57
 *
 * '@ @.'
 * '@ g.' 
 * '@ @.' levels: 34
 */

struct game_config {
	struct b6_entry entry;
	double pacman_speed;
	double ghosts_speed;
	double booster;
	double micro_booster_bonus;
	double small_booster_bonus;
	double large_booster_bonus;
	unsigned long int small_casino_award;
	unsigned long int large_casino_award;
	unsigned long int pacgum_score;
	unsigned long int jewel_score;
	unsigned long int ghost_score;
	unsigned long int ghost_score_limit;
	unsigned long int extra_life_score;
	unsigned long int shield_duration;
	unsigned long int shield_wear_out_duration;
	unsigned long int banquet_duration;
	unsigned long int diet_duration;
	unsigned long int zzz_duration;
	unsigned long int x2_duration;
	unsigned long int super_pacgum_duration;
	unsigned long int ghosts_recovery_duration;
	unsigned long int ghost_startup_interval;
	unsigned long int ghost_respawn_duration;
	unsigned long int bonus_enabled_duration;
	unsigned long int bonus_disabled_duration;
	unsigned long int pacman_slow_duration;
	unsigned long int pacman_fast_duration;
	unsigned long int ghosts_slow_duration;
	unsigned long int ghosts_fast_duration;
	unsigned long int quick_completion_limit;
};

extern struct b6_registry __game_config_registry;

static inline const struct game_config *get_default_game_config(void)
{
	struct b6_entry *entry = b6_get_first_entry(&__game_config_registry);
	b6_check(entry);
	return b6_cast_of(entry, struct game_config, entry);
}

static inline const struct game_config *lookup_game_config(
	const struct b6_utf8 *id)
{
	struct b6_entry *e = b6_lookup_registry(&__game_config_registry, id);
	return e ? b6_cast_of(e, struct game_config, entry) : NULL;
}

static inline const struct game_config *get_next_game_config(
	const struct game_config *self)
{
	struct b6_entry *entry = b6_walk_registry(&__game_config_registry,
						  &self->entry, B6_NEXT);
	if (!entry)
		entry = b6_get_first_entry(&__game_config_registry);
	return b6_cast_of(entry, struct game_config, entry);
}

struct game_casino {
	double value[3];
	double delta[3];
	unsigned int award;
};

enum game_pause_reason {
	GAME_PAUSED_READY, /* game paused when starting a new level */
	GAME_PAUSED_RETRY, /* game paused when retrying the current level */
	GAME_PAUSED_OTHER, /* game paused externally */
};

struct game {
	const struct game_ops *curr_ops;
	const struct game_ops *next_ops;
	struct b6_stopwatch stopwatch;
	struct layout_provider *layout_provider;
	struct layout layout;
	struct items items;
	struct level level;
	struct item_observer pacgum;
	struct item_observer super_pacgum;
	struct item_observer bonus;
	struct item_observer teleport[2];
	struct pacman pacman;
	struct ghost ghosts[4];
	struct b6_event_queue realtime_event_queue;
	struct b6_event_queue event_queue;
	void *event_queue_buffer[32];
	struct b6_fixed_allocator event_queue_allocator;
	struct game_event bonus_enabled;
	struct game_event bonus_disabled;
	struct game_event shield;
	struct game_event shield_wearing_out;
	struct game_event x2;
	struct game_event zzz;
	struct game_event zzz_recovering;
	struct game_event diet;
	struct game_event ghosts_vulnerable;
	struct game_event ghosts_recovering;
	struct game_event banquet;
	struct game_event pacman_fast;
	struct game_event pacman_slow;
	struct game_event ghosts_fast;
	struct game_event ghosts_slow;
	struct game_event introduce_ghost[4];
	struct b6_list infos;
	unsigned int extra_life_score;
	unsigned int pacgum_score;
	unsigned int ghost_score;
	unsigned int n;
	struct b6_list observers;
	unsigned long long int time;
	unsigned long long int quick_completion_limit;
	unsigned long int hold;
	const struct game_config *config;
	int rewind;
	int boost;
	unsigned int quick_completion_bonus;
	struct game_casino casino;
	enum game_pause_reason pause_reason;
};

struct game_observer {
	struct b6_dref dref;
	const struct game_observer_ops *ops;
};

struct game_observer_ops {
	void (*on_casino_launch)(struct game_observer*);
	void (*on_casino_update)(struct game_observer*);
	void (*on_casino_finish)(struct game_observer*);
	void (*on_score_change)(struct game_observer*);
	void (*on_score_bump)(struct game_observer*, unsigned int);
	void (*on_booster_charge)(struct game_observer*);
	void (*on_booster_micro_reload)(struct game_observer*);
	void (*on_booster_small_reload)(struct game_observer*);
	void (*on_booster_large_reload)(struct game_observer*);
	void (*on_booster_full)(struct game_observer*);
	void (*on_booster_empty)(struct game_observer*);
	void (*on_pacman_move)(struct game_observer*);
	void (*on_pacman_diet_begin)(struct game_observer*);
	void (*on_pacman_diet_end)(struct game_observer*);
	void (*on_extra_life)(struct game_observer*);
	void (*on_teleport)(struct game_observer*);
	void (*on_banquet_on)(struct game_observer*);
	void (*on_banquet_off)(struct game_observer*);
	void (*on_x2_on)(struct game_observer*);
	void (*on_x2_off)(struct game_observer*);
	void (*on_slow_pacman_on)(struct game_observer*);
	void (*on_slow_pacman_off)(struct game_observer*);
	void (*on_fast_pacman_on)(struct game_observer*);
	void (*on_fast_pacman_off)(struct game_observer*);
	void (*on_slow_ghosts_on)(struct game_observer*);
	void (*on_slow_ghosts_off)(struct game_observer*);
	void (*on_fast_ghosts_on)(struct game_observer*);
	void (*on_fast_ghosts_off)(struct game_observer*);
	void (*on_jewel_pickup)(struct game_observer*, int);
	void (*on_shield_pickup)(struct game_observer*);
	void (*on_shield_change)(struct game_observer*, int);
	void (*on_shield_empty)(struct game_observer*);
	void (*on_super_pacgum)(struct game_observer*, int);
	void (*on_zzz)(struct game_observer*, int);
	void (*on_wipeout)(struct game_observer*);
	void (*on_ghost_move)(struct game_observer*, const struct ghost*);
	void (*on_ghost_state_change)(struct game_observer*,
				      const struct ghost*);
	void (*on_level_touch)(struct game_observer*, struct place*,
			       struct item*);
	void (*on_level_init)(struct game_observer*);
	void (*on_level_exit)(struct game_observer*);
	void (*on_level_start)(struct game_observer*);
	void (*on_level_enter)(struct game_observer*);
	void (*on_level_leave)(struct game_observer*);
	void (*on_level_passed)(struct game_observer*);
	void (*on_level_failed)(struct game_observer*);
	void (*on_game_paused)(struct game_observer*);
	void (*on_game_resumed)(struct game_observer*);
};

static inline struct game_observer *setup_game_observer(
	struct game_observer *self, const struct game_observer_ops *ops)
{
	self->ops = ops;
	b6_reset_observer(&self->dref);
	return self;
}

static inline void add_game_observer(struct game *g, struct game_observer *o)
{
	b6_attach_observer(&g->observers, &o->dref);
}

static inline void del_game_observer(struct game_observer *o)
{
	b6_detach_observer(&o->dref);
}

extern int initialize_game(struct game *self,
			   const struct b6_clock *clock,
			   const struct game_config *config,
			   struct layout_provider *layout_provider,
			   unsigned int start_level);

extern void finalize_game(struct game *self);

static inline void add_game_hold(struct game *self) { self->hold += 1; }

static inline void remove_game_hold(struct game *self) { self->hold -= 1; }

extern int hold_game_for(struct game *self, struct game_event *event,
			 unsigned long int duration_us);

static inline enum game_pause_reason get_game_pause_reason(
	const struct game *self)
{
	return self->pause_reason;
}

static inline double get_normalized_booster(const struct game *self)
{
	return self->pacman.booster / self->config->booster;
}

static inline int game_level_completed(const struct game *self)
{
	return !self->level.items->pacgum.count;
}

struct game_ops {
	int (*init)(struct game*);
	void (*exit)(struct game*);
	void (*pause)(struct game*);
	void (*resume)(struct game*);
	void (*update)(struct game*);
	void (*submit)(struct game*, enum direction);
	void (*cancel)(struct game*, enum direction);
	void (*shield)(struct game*);
};

extern void abort_game(struct game *self);

static inline int play_game(struct game *self)
{
	if (self->hold)
		return !!self->hold;
	self->curr_ops = self->next_ops;
	self->next_ops = NULL;
	self->time = b6_get_clock_time(self->stopwatch.clock);
	return self->curr_ops->init(self) == 0;
}

static inline unsigned long long int update_game(struct game *self)
{
	if (self->curr_ops && self->curr_ops->update)
		self->curr_ops->update(self);
	b6_trigger_events(&self->realtime_event_queue,
			  b6_get_clock_time(self->stopwatch.clock));
	return self->time;
}

static inline void pause_game(struct game *self)
{
	if (!self->curr_ops)
		return;
	if (self->curr_ops->pause && !self->stopwatch.frozen) {
		self->pause_reason = GAME_PAUSED_OTHER;
		self->curr_ops->pause(self);
	}
}

static inline void resume_game(struct game *self)
{
	if (!self->curr_ops)
		return;
	if (self->curr_ops->resume && self->stopwatch.frozen)
		self->curr_ops->resume(self);
}

static inline void submit_pacman_move(struct game *self, enum direction d)
{
	if (self->curr_ops && self->curr_ops->submit)
		self->curr_ops->submit(self, d);
}

static inline void cancel_pacman_move(struct game *self, enum direction d)
{
	if (self->curr_ops && self->curr_ops->cancel)
		self->curr_ops->cancel(self, d);
}

extern void activate_game_bonus(struct game*, enum bonus_type);
extern void activate_game_booster(struct game*);
extern void deactivate_game_booster(struct game*);

static inline void activate_game_shield(struct game *self)
{
	if (self->curr_ops && self->curr_ops->shield)
		self->curr_ops->shield(self);
}

#endif /* GAME_H */
