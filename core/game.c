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

#include "lib/log.h"
#include "lib/rng.h"
#include "game.h"
#include "data.h"
#include "items.h"
#include "lib/init.h"
#include "lib/std.h"

#include <b6/cmdline.h>

static int cheat = 0;
b6_flag(cheat, bool);

#define __notify_game_observers(_game, _op, _args...) \
	b6_notify_observers(&(_game)->observers, game_observer, _op, ##_args)

#define __for_each_ghost(_self, _ghost) \
	for (_ghost = (_self)->ghosts, \
	     (void)((_self)==(struct game*)NULL), \
	     (void)((_ghost)==(struct ghost*)NULL); \
	     _ghost < (_self)->ghosts + b6_card_of((_self)->ghosts); \
	     _ghost += 1) \

#define __for_each_ghost_alive(_self, _ghost) \
	__for_each_ghost(_self, _ghost) \
		if (!ghost_is_alive(_ghost)) \
			continue; \
		else

static const struct game_ops enter_ops;
static const struct game_ops ready_ops;
static const struct game_ops retry_ops;
static const struct game_ops leave_ops;

static void notify_super_pacgum(struct game *self, int state)
{
	__notify_game_observers(self, on_super_pacgum, state);
}

static void notify_zzz(struct game *self, int state)
{
	__notify_game_observers(self, on_zzz, state);
}

static void notify_shield_change(struct game *self, int state)
{
	__notify_game_observers(self, on_shield_change, state);
}

static void notify_level_enter(struct game *self)
{
	__notify_game_observers(self, on_level_enter);
}

static void notify_level_leave(struct game *self)
{
	__notify_game_observers(self, on_level_leave);
}

static void notify_level_passed(struct game *self)
{
	__notify_game_observers(self, on_level_passed);
}

static void notify_level_failed(struct game *self)
{
	__notify_game_observers(self, on_level_failed);
}

static void reset_game_event(struct game *self, struct game_event *event,
			     const struct b6_event_ops *ops, const char *name)
{
	event->game = self;
	event->name = name;
	b6_reset_event(&event->event, ops);
}

static int game_event_is_pending(struct game_event *event)
{
	return b6_event_is_pending(&event->event);
}

static void defer_game_event(struct game_event *self, unsigned long int delay)
{
	struct b6_event *event = &self->event;
	struct game *game = self->game;
	struct b6_event_queue *event_queue = &game->event_queue;
	if (game_event_is_pending(self))
		b6_cancel_event(event_queue, event);
	b6_defer_event(event_queue, event, game->time + delay);
}

static void cancel_game_event(struct game_event *self)
{
	b6_cancel_event(&self->game->event_queue, &self->event);
}

static void change_ghost_state(struct game *self, struct ghost *ghost,
			       enum ghost_state state)
{
	set_ghost_state(ghost, state);
	__notify_game_observers(self, on_ghost_state_change, ghost);
}

static const struct game_ops *get_next_ops(struct game *self)
{
	return self->next_ops;
}

static void set_next_ops(struct game *self, const struct game_ops *ops)
{
	if (!self->next_ops && self->curr_ops && self->curr_ops->exit)
		self->curr_ops->exit(self);
	self->next_ops = ops;
}

static void extra_life(struct game *self)
{
	self->pacman.lifes += 1;
	__notify_game_observers(self, on_extra_life);
}

static void increase_game_score(struct game *self, unsigned int amount)
{
	self->pacman.score += amount;
	__notify_game_observers(self, on_score_change);
	while (self->pacman.score > self->extra_life_score) {
		extra_life(self);
		self->extra_life_score += self->config->extra_life_score;
	}
}

static void increase_game_score_and_bump(struct game *self, unsigned int amount)
{
	increase_game_score(self, amount);
	__notify_game_observers(self, on_score_bump, amount);
}

static void launch_casino(struct game *self)
{
	struct game_casino *casino = &self->casino;
	int i = b6_card_of(casino->delta);
	while (i--) {
		double delta = 4 + read_random_number_generator() * 8;
		casino->delta[i] += (int)delta;
	}
	casino->award = 0;
	__notify_game_observers(self, on_casino_launch);
}

static void update_casino(struct game *self, unsigned long long int dt)
{
	const double delta = (double)dt / 1000 / 200;
	struct game_casino *casino = &self->casino;
	int i, j, updated, rolling, count[] = {0, 0, 0, 0};
	if (!dt)
		return;
	for (i = b6_card_of(casino->value), updated = rolling = 0; i--;) {
		if (!casino->delta[i])
			continue;
		updated += 1;
		if (delta < casino->delta[i]) {
			casino->value[i] += delta;
			casino->delta[i] -= delta;
			rolling += 1;
		} else {
			double ceil;
			casino->value[i] += casino->delta[i];
			casino->delta[i] = 0;
			ceil = (int)casino->value[i];
			if (casino->value[i] - ceil < .5)
				casino->value[i] = ceil;
			else
				casino->value[i] = ceil + 1;
		}
		while (casino->value[i] >= 4)
			casino->value[i] -= 4;
	}
	if (!updated)
		return;
	__notify_game_observers(self, on_casino_update);
	if (rolling)
		return;
	/* compute award */
	for (i = b6_card_of(casino->value), j = 0; i--;) {
		int k = casino->value[i];
		if (++count[k] > count[j])
			j = k;
	}
	switch (count[j]) {
	case 3:
		casino->award = self->config->large_casino_award + 100 * j;
		break;
	case 2:
		casino->award = self->config->small_casino_award + 100 * j;
		break;
	default:
		casino->award = 100;
	}
	__notify_game_observers(self, on_casino_finish);
	increase_game_score(self, casino->award);
}

static void finish_casino(struct game *self)
{
	update_casino(self, 1000 * 200 * 12);
}

static void maybe_award_quick_completion_bonus(struct game *self)
{
	if (self->quick_completion_limit <= self->time)
		self->quick_completion_bonus = 0;
	else
		self->quick_completion_bonus = (self->quick_completion_limit -
						self->time) / 1000000ULL * 10;
}

static void game_passed(struct game *self)
{
	finish_casino(self);
	self->rewind = 0;
	set_next_ops(self, &leave_ops);
	notify_level_passed(self);
	maybe_award_quick_completion_bonus(self);
}

static void game_rewind(struct game *self)
{
	finish_casino(self);
	self->rewind = 1;
	set_next_ops(self, &leave_ops);
	notify_level_passed(self);
	maybe_award_quick_completion_bonus(self);
}

static void game_failed(struct game *self)
{
	if (game_level_completed(self))
		return;
	finish_casino(self);
	if (game_event_is_pending(&self->shield))
		return;
	if (--self->pacman.lifes < 0)
		set_next_ops(self, &leave_ops);
	else
		set_next_ops(self, &retry_ops);
	notify_level_failed(self);
}

static void increase_shields(struct game *self)
{
	self->pacman.shields += 1;
	__notify_game_observers(self, on_shield_pickup);
}

static int decrease_shields(struct game *self)
{
	int ok = !!self->pacman.shields;
	if (ok)
		self->pacman.shields -= 1;
	else
		__notify_game_observers(self, on_shield_empty);
	return ok;
}

static void enable_super_pacgum(struct game *self)
{
	defer_game_event(&self->ghosts_vulnerable,
			 self->config->super_pacgum_duration);
	defer_game_event(&self->ghosts_recovering,
			 self->config->super_pacgum_duration -
			 self->config->ghosts_recovery_duration);
}

static void touch_level(struct game *self, struct place *place,
			struct item *item)
{
	item = set_level_place_item(&self->level, place, item);
	__notify_game_observers(self, on_level_touch, place, item);
}

static void eat_pacgum(struct game *self, struct place *place,
		       struct item *item)
{
	if (get_next_ops(self) == &leave_ops)
		return;
	touch_level(self, place, clone_empty(self->level.items));
	if (game_level_completed(self))
		game_passed(self);
}

static void ghost_eats_pacgum(struct item_observer *observer,
			      struct mobile *mobile,
			      struct item *item)
{
	struct game *self = b6_cast_of(observer, struct game, pacgum);
	struct ghost *ghost = b6_cast_of(mobile, struct ghost, mobile);
	if (game_event_is_pending(&self->banquet) && ghost_is_alive(ghost))
		eat_pacgum(self, mobile->curr, item);
}

static void pacman_eats_pacgum(struct item_observer *observer,
			       struct mobile *mobile,
			       struct item *item)
{
	struct game *self = b6_cast_of(observer, struct game, pacgum);
	if (!game_event_is_pending(&self->diet)) {
		increase_game_score(self, self->pacgum_score);
		eat_pacgum(self, mobile->curr, item);
	}
}

static void pacman_eats_super_pacgum(struct item_observer *observer,
				     struct mobile *mobile,
				     struct item *item)
{
	struct game *self = b6_cast_of(observer, struct game, super_pacgum);
	struct place *place = mobile->curr;
	touch_level(self, place, clone_empty(self->level.items));
	enable_super_pacgum(self);
}

static void mobile_enters_teleport(struct item_observer *observer,
				   struct mobile *mobile,
				   struct item *item)
{
	mobile->curr = get_teleport_destination(item);
	mobile->next = NULL;
}

static void pacman_enters_teleport(struct item_observer *observer,
				   struct mobile *mobile,
				   struct item *item)
{
	struct teleport *t = b6_cast_of(item, struct teleport, item);
	struct teleport *s = t->teleport;
	struct game *self = b6_cast_of(observer, struct game, teleport[t > s]);
	mobile_enters_teleport(observer, mobile, item);
	__notify_game_observers(self, on_teleport);
}

static void on_defer_ghosts_vulnerable(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	struct ghost *ghost;
	__for_each_ghost(game, ghost)
		if (get_ghost_state(ghost) == GHOST_HUNTER)
			change_ghost_state(game, ghost, GHOST_AFRAID);
	notify_super_pacgum(game, 1);
}

static void on_cancel_ghosts_vulnerable(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	struct ghost *ghost;
	__for_each_ghost(game, ghost)
		if (get_ghost_state(ghost) == GHOST_AFRAID)
			change_ghost_state(game, ghost, GHOST_HUNTER);
	game->ghost_score = game->config->ghost_score;
}

static void on_trigger_ghosts_vulnerable(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	on_cancel_ghosts_vulnerable(up);
	notify_super_pacgum(event->game, -1);
}

static void on_trigger_ghosts_recovering(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	notify_super_pacgum(event->game, 0);
}

static void on_start_banquet(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	__notify_game_observers(event->game, on_banquet_on);
}

static void on_stop_banquet(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	__notify_game_observers(event->game, on_banquet_off);
}

static void on_start_zzz(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	struct ghost *ghost;
	__for_each_ghost_alive(game, ghost)
		change_ghost_state(game, ghost, GHOST_FROZEN);
	notify_zzz(game, 1);
}

static void on_stop_zzz(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	enum ghost_state state =
		game_event_is_pending(&game->ghosts_vulnerable) ?
		GHOST_AFRAID : GHOST_HUNTER;
	struct ghost *ghost;
	__for_each_ghost_alive(game, ghost)
		change_ghost_state(game, ghost, state);
	notify_zzz(game, -1);
}

static void on_trigger_zzz_recovering(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	notify_zzz(event->game, 0);
}

static void on_start_diet(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	__notify_game_observers(event->game, on_pacman_diet_begin);
}

static void on_stop_diet(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	__notify_game_observers(event->game, on_pacman_diet_end);
}

static void on_start_x2(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	game->pacgum_score = game->config->pacgum_score * 2;
	__notify_game_observers(event->game, on_x2_on);
}

static void on_stop_x2(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	game->pacgum_score = game->config->pacgum_score;
	__notify_game_observers(event->game, on_x2_off);
}

static void on_start_pacman_slow(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	__notify_game_observers(game, on_slow_pacman_on);
}

static void on_stop_pacman_slow(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	__notify_game_observers(game, on_slow_pacman_off);
}

static void on_start_pacman_fast(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	__notify_game_observers(game, on_fast_pacman_on);
}

static void on_stop_pacman_fast(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	__notify_game_observers(game, on_fast_pacman_off);
}

static void on_start_shield(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	notify_shield_change(game, 1);
}

static void on_stop_shield(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	notify_shield_change(event->game, -1);
}

static void on_wear_shield_out(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	notify_shield_change(event->game, 0);
}

static void alter_ghosts_speed(struct game *self, double speed)
{
	struct ghost *ghost;
	__for_each_ghost_alive(self, ghost)
		set_mobile_speed(&ghost->mobile, speed);
}

static void restore_ghosts_speed(struct game *self)
{
	struct ghost *ghost;
	__for_each_ghost(self, ghost)
		set_mobile_speed(&ghost->mobile, self->config->ghosts_speed);
}

static void on_start_ghosts_slow(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	if (game_event_is_pending(&game->ghosts_fast))
		cancel_game_event(&game->ghosts_fast);
	__notify_game_observers(game, on_slow_ghosts_on);
	alter_ghosts_speed(game, game->config->ghosts_speed / 1.5);
}

static void on_stop_ghosts_slow(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	restore_ghosts_speed(game);
	__notify_game_observers(game, on_slow_ghosts_off);
}

static void on_start_ghosts_fast(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	if (game_event_is_pending(&game->ghosts_slow))
		cancel_game_event(&game->ghosts_slow);
	__notify_game_observers(game, on_fast_ghosts_on);
	alter_ghosts_speed(game, game->config->ghosts_speed * 1.5);
}

static void on_stop_ghosts_fast(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	restore_ghosts_speed(game);
	__notify_game_observers(game, on_fast_ghosts_off);
}

static void on_new_ghost(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	int n = event - game->introduce_ghost;
	struct ghost *ghost = &game->ghosts[n];
	reset_ghost(ghost, &game->level);
	change_ghost_state(game, ghost, GHOST_HUNTER);
	set_mobile_speed(&ghost->mobile, game->config->ghosts_speed);
	if (game_event_is_pending(&game->zzz))
		set_ghost_state(ghost, GHOST_FROZEN);
	else if (game_event_is_pending(&game->ghosts_vulnerable))
		set_ghost_state(ghost, GHOST_AFRAID);
}

static void on_trigger_bonus_enabled(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	struct place *place = game->level.bonus_place;
	struct items *items = game->level.items;
	struct item *item = clone_bonus(items);
	set_level_place_item(&game->level, place, clone_empty(items));
	touch_level(game, place, item);
	defer_game_event(&game->bonus_disabled,
			 game->config->bonus_enabled_duration);
	if (game->pacman.mobile.curr == place)
		pacman_visits_item(&game->pacman.mobile, item);
}

static void on_trigger_bonus_disabled(struct b6_event *up)
{
	struct game_event *event = b6_cast_of(up, struct game_event, event);
	struct game *game = event->game;
	touch_level(game, game->level.bonus_place,
		    clone_empty(game->level.items));
	defer_game_event(&game->bonus_enabled,
			 game->config->bonus_disabled_duration);
}

static int get_free_jewel(struct game *self)
{
	int jewel;
	unsigned int mask;
	for (jewel = 0, mask = 1; jewel < 7; jewel += 1, mask *= 2)
		if (!(self->pacman.jewels & mask))
			return jewel;
	return 0;
}

static void catch_jewel_bonus(struct game *self, int jewel)
{
	increase_game_score_and_bump(self, self->config->jewel_score);
	if (self->pacman.jewels == 0x7f)
		self->pacman.jewels = 0;
	self->pacman.jewels |= 1 << jewel;
	__notify_game_observers(self, on_jewel_pickup, jewel);
	if (self->pacman.jewels == 0x7f)
		extra_life(self);
}

static void catch_wipeout_bonus(struct game *self)
{
	struct ghost *ghost;
	unsigned long long int time;
	if (!self->level.ghosts_home)
		return;
	__notify_game_observers(self, on_wipeout);
	time = self->config->ghost_respawn_duration;
	__for_each_ghost_alive(self, ghost) {
		int i = ghost - self->ghosts;
		change_ghost_state(self, ghost, GHOST_OUT);
		reset_ghost(ghost, &self->level);
		defer_game_event(&self->introduce_ghost[i], time);
		time += self->config->ghost_startup_interval;
	}
}

static void catch_casino_bonus(struct game *self)
{
	finish_casino(self);
	launch_casino(self);
}

static void notify_booster_full(const struct game *self)
{
	__notify_game_observers(self, on_booster_full);
}

static void activate_bonus(struct game *self, enum bonus_type type)
{
	switch (type) {
	case JEWELS_BONUS: catch_jewel_bonus(self, get_free_jewel(self)); break;
	case JEWEL1_BONUS: catch_jewel_bonus(self, 0); break;
	case JEWEL2_BONUS: catch_jewel_bonus(self, 1); break;
	case JEWEL3_BONUS: catch_jewel_bonus(self, 2); break;
	case JEWEL4_BONUS: catch_jewel_bonus(self, 3); break;
	case JEWEL5_BONUS: catch_jewel_bonus(self, 4); break;
	case JEWEL6_BONUS: catch_jewel_bonus(self, 5); break;
	case JEWEL7_BONUS: catch_jewel_bonus(self, 6); break;
	case SINGLE_BOOSTER_BONUS:
		if (self->pacman.booster >= self->config->booster) {
			notify_booster_full(self);
			break;
		}
		reload_booster(&self->pacman, self->config->small_booster_bonus,
			       self->config->booster);
		__notify_game_observers(self, on_booster_small_reload);
		break;
	case DOUBLE_BOOSTER_BONUS:
		if (self->pacman.booster >= self->config->booster) {
			notify_booster_full(self);
			break;
		}
		reload_booster(&self->pacman, self->config->large_booster_bonus,
			       self->config->booster);
		__notify_game_observers(self, on_booster_large_reload);
		break;
	case SUPER_PACGUM_BONUS:
		enable_super_pacgum(self);
		break;
	case EXTRA_LIFE_BONUS:
		extra_life(self);
		break;
	case PACMAN_SLOW_BONUS:
		defer_game_event(&self->pacman_slow,
				 self->config->pacman_slow_duration);
		break;
	case PACMAN_FAST_BONUS:
		defer_game_event(&self->pacman_fast,
				 self->config->pacman_fast_duration);
		break;
	case SHIELD_BONUS:
		increase_shields(self);
		__notify_game_observers(self, on_shield_pickup);
		break;
	case DIET_BONUS:
		defer_game_event(&self->diet, self->config->diet_duration);
		break;
	case SUICIDE_BONUS:
		game_failed(self);
		break;
	case GHOSTS_BANQUET_BONUS:
		defer_game_event(&self->banquet,
				 self->config->banquet_duration);
		break;
	case GHOSTS_WIPEOUT_BONUS:
		catch_wipeout_bonus(self);
		break;
	case GHOSTS_SLOW_BONUS:
		defer_game_event(&self->ghosts_slow,
				 self->config->ghosts_slow_duration);
		break;
	case GHOSTS_FAST_BONUS:
		defer_game_event(&self->ghosts_fast,
				 self->config->ghosts_fast_duration);
		break;
	case ZZZ_BONUS:
		defer_game_event(&self->zzz, self->config->zzz_duration);
		defer_game_event(&self->zzz_recovering,
				 self->config->zzz_duration -
				 self->config->ghosts_recovery_duration);
		break;
	case X2_BONUS:
		defer_game_event(&self->x2, self->config->x2_duration);
		break;
	case MINUS_BONUS:
		game_rewind(self);
		break;
	case PLUS_BONUS:
		game_passed(self);
		break;
	case CASINO_BONUS:
		catch_casino_bonus(self);
		break;
	default:
		break;
	}
}

void activate_game_bonus(struct game *self, enum bonus_type type)
{
	if (cheat)
		activate_bonus(self, type);
}

static void pacman_catches_bonus(struct item_observer *observer,
				 struct mobile *mobile, struct item *item)
{
	struct game *self = b6_cast_of(observer, struct game, bonus);
	activate_bonus(self, unveil_bonus_contents(item));
	defer_game_event(&self->bonus_disabled, 0);
}

static void add_hold_event(struct b6_event *event)
{
	add_game_hold(b6_cast_of(event, struct game_event, event)->game);
}

static void remove_hold_event(struct b6_event *event)
{
	remove_game_hold(b6_cast_of(event, struct game_event, event)->game);
}

int hold_game_for(struct game *self, struct game_event *event,
		  unsigned long int duration)
{
	static const struct b6_event_ops ops = {
		.defer = add_hold_event,
		.cancel = remove_hold_event,
		.trigger = remove_hold_event,
	};
	if (game_event_is_pending(event))
		return -1;
	reset_game_event(self, event, &ops, "hold");
	b6_defer_event(&event->game->realtime_event_queue, &event->event,
		       b6_get_clock_time(self->stopwatch.clock) + duration);
	return 0;
}

void abort_game(struct game *self)
{
	self->pacman.lifes = -1;
	if (self->curr_ops != &leave_ops)
		set_next_ops(self, &leave_ops);
}

static void pacman_eats_ghost(struct game *self, struct ghost *ghost)
{
	int i = ghost - self->ghosts;
	if (game_event_is_pending(&self->ghosts_recovering) ||
	    game_event_is_pending(&self->zzz_recovering) ||
	    self->ghost_score > self->config->ghost_score_limit)
		increase_game_score_and_bump(self, self->ghost_score);
	else
		increase_game_score_and_bump(self,
					     self->config->ghost_score_limit);
	if (self->ghost_score <= self->config->ghost_score_limit)
		self->ghost_score *= 2;
	change_ghost_state(self, ghost, GHOST_ZOMBIE);
	set_mobile_speed(&ghost->mobile, self->config->ghosts_speed * 1.5);
	defer_game_event(&self->introduce_ghost[i],
			 self->config->ghost_respawn_duration);
}

static double update_pacman(struct game *self)
{
	double d = update_mobile(&self->pacman.mobile);
	__notify_game_observers(self, on_pacman_move);
	return d;
}

static void update_ghost(struct game *self, struct ghost *ghost)
{
	update_mobile(&ghost->mobile);
	__notify_game_observers(self, on_ghost_move, ghost);
}

static void do_update_at(struct game *self, unsigned long long int now)
{
	struct ghost *ghost;
	struct pacman *pacman = &self->pacman;
	double speed = self->config->pacman_speed;
	double d;
	int boost = 0;
	update_casino(self, now - self->time);
	b6_trigger_events(&self->event_queue, now);
	self->time = now;
	if (game_event_is_pending(&self->pacman_fast))
		speed += self->config->pacman_speed * .66;
	if (game_event_is_pending(&self->pacman_slow))
		speed -= self->config->pacman_speed * .33;
	if (self->boost && speed <= self->config->pacman_speed) {
		if (pacman->booster > 0) {
			boost = 1;
			speed += self->config->pacman_speed * .66;
		} else
			__notify_game_observers(self, on_booster_empty);
	}
	set_mobile_speed(&self->pacman.mobile, speed);
	d = update_pacman(self);
	set_mobile_speed(&self->pacman.mobile, self->config->pacman_speed);
	if (boost && d > 0) {
		charge_booster(&self->pacman, d);
		__notify_game_observers(self, on_booster_charge);
	}
	__for_each_ghost(self, ghost) {
		struct mobile *mobile = &ghost->mobile;
		enum ghost_state state = get_ghost_state(ghost);
		double dx, dy;
		if (state == GHOST_OUT)
			continue;
		update_ghost(self, ghost);
		if (state == GHOST_ZOMBIE && mobile_is_stopped(mobile)) {
			change_ghost_state(self, ghost, GHOST_OUT);
			set_mobile_speed(&ghost->mobile,
					 self->config->ghosts_speed);
			reset_ghost(ghost, &self->level);
			continue;
		}
		dx = mobile->x - pacman->mobile.x;
		dy = mobile->y - pacman->mobile.y;
		if (dx * dx + dy * dy >= 1)
			continue;
		if (state == GHOST_AFRAID || state == GHOST_FROZEN)
			pacman_eats_ghost(self, ghost);
		else if (state == GHOST_HUNTER)
			game_failed(self);
	}
}

static void do_update(struct game *self)
{
	unsigned long long int now = b6_get_stopwatch_time(&self->stopwatch);
	unsigned long long int delta = now - self->time;
	if (!get_next_ops(self) && !self->stopwatch.frozen && delta >= 30000) {
		logf_w("game catching up: %llu", delta);
		do
			do_update_at(self, self->time + 10000);
		while (now - self->time > 10000);
	}
	do_update_at(self, now);
}

static void do_pause(struct game *self)
{
	b6_pause_stopwatch(&self->stopwatch);
	__notify_game_observers(self, on_game_paused);
}

static void do_resume(struct game *self)
{
	b6_resume_stopwatch(&self->stopwatch);
	__notify_game_observers(self, on_game_resumed);
}

static int init_level(struct game *self, int n)
{
	int error = open_level(&self->level, &self->layout);
	if (error) {
		struct ofstream ofs;
		logf_w("skipping rejected layout #%d: %d", n, error);
		initialize_ofstream_with_fp(&ofs, stderr, 0);
		print_layout(&self->layout, &ofs.ostream);
		finalize_ofstream(&ofs);
	} else
		self->n = n;
	return error;
}

static int load_level(struct game *self, unsigned int n)
{
	return get_layout_from_provider(self->layout_provider, n,
					&self->layout);
}

static void startup(struct game *self)
{
	struct ghost *ghost;
	self->pause_reason = self->curr_ops == &retry_ops ? GAME_PAUSED_RETRY :
		GAME_PAUSED_READY;
	do_pause(self);
	self->time = b6_get_stopwatch_time(&self->stopwatch);
	reset_pacman(&self->pacman, &self->level);
	self->ghost_score = self->config->ghost_score;
	if (self->level.ghosts_home) {
		unsigned long long int duration = 0;
		initialize_ghost_strategies(self);
		__for_each_ghost(self, ghost) {
			int i = ghost - self->ghosts;
			reset_ghost(ghost, &self->level);
			defer_game_event(&self->introduce_ghost[i], duration);
			duration += self->config->ghost_startup_interval;
		}
	}
	if (self->level.bonus_place)
		defer_game_event(&self->bonus_enabled, 0);
	/* Make sure event deferred to now are immediately executed. */
	b6_trigger_events(&self->event_queue, self->time);
	update_pacman(self);
	__for_each_ghost(self, ghost)
		if (get_ghost_state(ghost) != GHOST_OUT)
			update_ghost(self, ghost);
	__notify_game_observers(self, on_level_start);
}

static int init_enter(struct game *self)
{
	if (is_level_open(&self->level)) {
		__notify_game_observers(self, on_level_exit);
		close_level(&self->level);
	}
	if (self->pacman.lifes < 0)
		return -1;
	if (self->rewind) {
		int n = self->n;
		do {
			int error;
			if (!--n) {
				if ((error = init_level(self, self->n)))
					return error;
				break;
			}
			if ((error = load_level(self, n)))
				return error;
		} while (init_level(self, n));
	} else {
		int n = self->n;
		do {
			int error = load_level(self, ++n);
			if (error)
				return error;
		} while (init_level(self, n));
	}
	__notify_game_observers(self, on_level_init);
	startup(self);
	notify_level_enter(self);
	set_next_ops(self, &ready_ops);
	return 0;
}

static int init_ready(struct game *self)
{
	b6_assert(!self->hold);
	add_game_hold(self);
	unlock_mobile(&self->pacman.mobile);
	if (self->quick_completion_bonus) {
		increase_game_score(self, self->quick_completion_bonus);
		self->quick_completion_bonus = 0;
	}
	self->quick_completion_limit = b6_get_stopwatch_time(&self->stopwatch) +
		self->config->quick_completion_limit;
	if (self->pacman.booster < self->config->booster) {
		reload_booster(&self->pacman, self->config->micro_booster_bonus,
			       self->config->booster);
		__notify_game_observers(self, on_booster_micro_reload);
	}
	return 0;
}

static void exit_ready(struct game *self)
{
	struct ghost *ghost;
	remove_game_hold(self);
	b6_cancel_all_events(&self->realtime_event_queue);
	b6_cancel_all_events(&self->event_queue);
	__for_each_ghost(self, ghost)
		change_ghost_state(self, ghost, GHOST_OUT);
	lock_mobile(&self->pacman.mobile);
}

static int init_retry(struct game *self)
{
	startup(self);
	b6_assert(!self->hold);
	add_game_hold(self);
	unlock_mobile(&self->pacman.mobile);
	return 0;
}

static void exit_retry(struct game *self)
{
	exit_ready(self);
}

static int init_leave(struct game *self)
{
	notify_level_leave(self);
	set_next_ops(self, &enter_ops);
	return 0;
}

void activate_game_booster(struct game *self)
{
	if (!self->boost)
		self->boost = 1;
}

void deactivate_game_booster(struct game *self)
{
	if (self->boost)
		self->boost = 0;
}

static void do_shield(struct game *self)
{
	if (game_event_is_pending(&self->shield_wearing_out))
		return;
	if (self->next_ops || self->stopwatch.frozen)
		return;
	if (!decrease_shields(self))
		return;
	defer_game_event(&self->shield_wearing_out,
			 self->config->shield_duration -
			 self->config->shield_wear_out_duration);
	defer_game_event(&self->shield, self->config->shield_duration);
}

static void do_submit(struct game *self, enum direction d)
{
	resume_game(self);
	submit_mobile_move(&self->pacman.mobile, d);
}

static void do_cancel(struct game *self, enum direction d)
{
	cancel_mobile_move(&self->pacman.mobile, d);
}

static const struct game_ops enter_ops = {
	.init = init_enter,
};
static const struct game_ops ready_ops = {
	.init = init_ready,
	.exit = exit_ready,
	.pause = do_pause,
	.resume = do_resume,
	.update = do_update,
	.submit = do_submit,
	.cancel = do_cancel,
	.shield = do_shield,
};
static const struct game_ops retry_ops = {
	.init = init_retry,
	.exit = exit_retry,
	.pause = do_pause,
	.resume = do_resume,
	.update = do_update,
	.submit = do_submit,
	.cancel = do_cancel,
	.shield = do_shield,
};
static const struct game_ops leave_ops = {
	.init = init_leave,
};

#define RESET_EVENT(self, event, ops) \
	reset_game_event(self, &self->event, ops, #event)

int initialize_game(struct game *self,
		    const struct b6_clock *clock,
		    const struct game_config *config,
		    struct layout_provider *layout_provider,
		    unsigned int start_level)
{
	static const struct b6_event_ops bonus_enabled_ops = {
		.trigger = on_trigger_bonus_enabled,
	};
	static const struct b6_event_ops bonus_disabled_ops = {
		.trigger = on_trigger_bonus_disabled,
	};
	static const struct b6_event_ops ghosts_vulnerable_ops = {
		.trigger = on_trigger_ghosts_vulnerable,
		.cancel = on_cancel_ghosts_vulnerable,
		.defer = on_defer_ghosts_vulnerable,
	};
	static const struct b6_event_ops ghosts_recovering_ops = {
		.trigger = on_trigger_ghosts_recovering,
	};
	static const struct b6_event_ops introduce_ghost_ops = {
		.trigger = on_new_ghost,
	};
	static const struct b6_event_ops shield_ops = {
		.trigger = on_stop_shield,
		.cancel = on_stop_shield,
		.defer = on_start_shield,
	};
	static const struct b6_event_ops shield_wearing_out_ops = {
		.trigger = on_wear_shield_out,
	};
	static const struct b6_event_ops banquet_ops = {
		.defer = on_start_banquet,
		.cancel = on_stop_banquet,
		.trigger = on_stop_banquet,
	};
	static const struct b6_event_ops zzz_ops = {
		.defer = on_start_zzz,
		.cancel = on_stop_zzz,
		.trigger = on_stop_zzz,
	};
	static const struct b6_event_ops zzz_recovering_ops = {
		.trigger = on_trigger_zzz_recovering,
	};
	static const struct b6_event_ops diet_ops = {
		.defer = on_start_diet,
		.cancel = on_stop_diet,
		.trigger = on_stop_diet,
	};
	static const struct b6_event_ops x2_ops = {
		.defer = on_start_x2,
		.cancel = on_stop_x2,
		.trigger = on_stop_x2,
	};
	static const struct b6_event_ops pacman_slow_ops = {
		.defer = on_start_pacman_slow,
		.cancel = on_stop_pacman_slow,
		.trigger = on_stop_pacman_slow,
	};
	static const struct b6_event_ops pacman_fast_ops = {
		.defer = on_start_pacman_fast,
		.cancel = on_stop_pacman_fast,
		.trigger = on_stop_pacman_fast,
	};
	static const struct b6_event_ops ghosts_slow_ops = {
		.defer = on_start_ghosts_slow,
		.cancel = on_stop_ghosts_slow,
		.trigger = on_stop_ghosts_slow,
	};
	static const struct b6_event_ops ghosts_fast_ops = {
		.defer = on_start_ghosts_fast,
		.cancel = on_stop_ghosts_fast,
		.trigger = on_stop_ghosts_fast,
	};
	static const struct item_observer_ops pacgum_ops = {
		.on_pacman_visit = pacman_eats_pacgum,
		.on_ghost_visit = ghost_eats_pacgum,
	};
	static const struct item_observer_ops super_pacgum_ops = {
		.on_pacman_visit = pacman_eats_super_pacgum,
	};
	static const struct item_observer_ops bonus_ops = {
		.on_pacman_visit = pacman_catches_bonus,
	};
	static const struct item_observer_ops teleport_ops = {
		.on_pacman_visit = pacman_enters_teleport,
		.on_ghost_visit = mobile_enters_teleport,
	};
	struct ghost *ghost;
	int i;
	b6_setup_stopwatch(&self->stopwatch, clock);
	b6_reset_fixed_allocator(&self->event_queue_allocator,
				 &self->event_queue_buffer,
				 sizeof(self->event_queue_buffer));
	b6_initialize_event_queue(&self->realtime_event_queue,
				  &b6_std_allocator);
	b6_initialize_event_queue(&self->event_queue,
				  &self->event_queue_allocator.allocator);
	RESET_EVENT(self, bonus_enabled, &bonus_enabled_ops);
	RESET_EVENT(self, bonus_disabled, &bonus_disabled_ops);
	RESET_EVENT(self, shield, &shield_ops);
	RESET_EVENT(self, shield_wearing_out, &shield_wearing_out_ops);
	RESET_EVENT(self, diet, &diet_ops);
	RESET_EVENT(self, zzz, &zzz_ops);
	RESET_EVENT(self, zzz_recovering, &zzz_recovering_ops);
	RESET_EVENT(self, x2, &x2_ops);
	RESET_EVENT(self, ghosts_vulnerable, &ghosts_vulnerable_ops);
	RESET_EVENT(self, ghosts_recovering, &ghosts_recovering_ops);
	RESET_EVENT(self, banquet, &banquet_ops);
	RESET_EVENT(self, ghosts_slow, &ghosts_slow_ops);
	RESET_EVENT(self, ghosts_fast, &ghosts_fast_ops);
	RESET_EVENT(self, pacman_slow, &pacman_slow_ops);
	RESET_EVENT(self, pacman_fast, &pacman_fast_ops);
	__for_each_ghost(self, ghost) {
		static const char *event_name[] = {
			"introduce_ghost_1",
			"introduce_ghost_2",
			"introduce_ghost_3",
			"introduce_ghost_4"
		};
		i = ghost - self->ghosts;
		initialize_ghost(ghost, i, config->ghosts_speed,
				 &self->stopwatch.up);
		reset_game_event(self, &self->introduce_ghost[i],
				 &introduce_ghost_ops, event_name[i]);
	}
	b6_list_initialize(&self->observers);
	initialize_items(&self->items);
	initialize_item_observer(&self->pacgum, &pacgum_ops);
	add_item_observer(&self->items.pacgum.item, &self->pacgum);
	initialize_item_observer(&self->super_pacgum, &super_pacgum_ops);
	add_item_observer(&self->items.super_pacgum.item, &self->super_pacgum);
	initialize_item_observer(&self->bonus, &bonus_ops);
	add_item_observer(&self->items.bonus.item, &self->bonus);
	initialize_item_observer(&self->teleport[0], &teleport_ops);
	add_item_observer(&self->items.teleport[0].item, &self->teleport[0]);
	initialize_item_observer(&self->teleport[1], &teleport_ops);
	add_item_observer(&self->items.teleport[1].item, &self->teleport[1]);
	initialize_level(&self->level, &self->items);
	self->n = start_level;
	self->time = 1;
	initialize_pacman(&self->pacman, &self->stopwatch.up,
			  config->pacman_speed, config->booster);
	self->pacman.booster = config->booster;
	self->hold = 0;
	self->layout_provider = layout_provider;
	self->config = config;
	self->extra_life_score = config->extra_life_score;
	self->pacgum_score = config->pacgum_score;
	self->curr_ops = NULL;
	self->next_ops = &enter_ops;
	self->rewind = 0;
	self->boost = 0;
	self->quick_completion_bonus = 0;
	self->casino.value[0] = 0; self->casino.delta[0] = 0;
	self->casino.value[1] = 0; self->casino.delta[1] = 0;
	self->casino.value[2] = 0; self->casino.delta[2] = 0;
	return 0;
}

void finalize_game(struct game *self)
{
	b6_finalize_event_queue(&self->realtime_event_queue);
	b6_finalize_event_queue(&self->event_queue);
	finalize_level(&self->level);
}

B6_REGISTRY_DEFINE(__game_config_registry);

static struct game_config slow_game_config = {
	.pacman_speed = 7.5,
	.ghosts_speed = 5,
	.booster = 100,
	.micro_booster_bonus = 10,
	.small_booster_bonus = 20,
	.large_booster_bonus = 40,
	.small_casino_award = 300UL,
	.large_casino_award = 900UL,
	.pacgum_score = 10UL,
	.jewel_score = 200UL,
	.ghost_score = 100UL,
	.ghost_score_limit = 400UL,
	.extra_life_score = 30000UL,
	.shield_duration = 9000000UL,
	.shield_wear_out_duration = 2000000UL,
	.banquet_duration = 7500000UL,
	.diet_duration = 7500000UL,
	.zzz_duration = 12000000UL,  /* FIXME: verify */
	.x2_duration = 12000000UL,
	.super_pacgum_duration = 10000000UL,
	.ghosts_recovery_duration = 3000000UL,
	.ghost_startup_interval = 7000000UL,
	.ghost_respawn_duration = 13000000UL,
	.bonus_enabled_duration = 9000000UL,
	.bonus_disabled_duration = 9000000UL,
	.pacman_slow_duration = 6000000UL,
	.pacman_fast_duration = 6000000UL,
	.ghosts_slow_duration = 6000000UL,
	.ghosts_fast_duration = 6000000UL,
	.quick_completion_limit = 60000000UL,
};

static struct game_config fast_game_config = {
	.pacman_speed = 11,
	.ghosts_speed = 11.l / 1.5,
	.booster = 100,
	.micro_booster_bonus = 10,
	.small_booster_bonus = 20,
	.large_booster_bonus = 40,
	.small_casino_award = 300UL,
	.large_casino_award = 900UL,
	.pacgum_score = 10UL,
	.jewel_score = 200UL,
	.ghost_score = 100UL,
	.ghost_score_limit = 400UL,
	.extra_life_score = 30000UL,
	.shield_duration = 6000000UL,
	.shield_wear_out_duration = 2000000UL,
	.banquet_duration = 5000000UL,
	.diet_duration = 5000000UL,
	.zzz_duration = 8000000UL,  /* FIXME: verify */
	.x2_duration = 8000000UL,
	.super_pacgum_duration = 10000000UL,
	.ghosts_recovery_duration = 3000000UL,
	.ghost_startup_interval = 5000000UL,
	.ghost_respawn_duration = 13000000UL,
	.bonus_enabled_duration = 6000000UL,
	.bonus_disabled_duration = 6000000UL,
	.pacman_slow_duration = 6000000UL,
	.pacman_fast_duration = 6000000UL,
	.ghosts_slow_duration = 6000000UL,
	.ghosts_fast_duration = 6000000UL,
	.quick_completion_limit = 60000000UL,
};

static int register_game_configs(void)
{
	b6_register(&__game_config_registry, &slow_game_config.entry,
		    B6_UTF8("slow"));
	b6_register(&__game_config_registry, &fast_game_config.entry,
		    B6_UTF8("fast"));
	return 0;
}
register_init(register_game_configs);
