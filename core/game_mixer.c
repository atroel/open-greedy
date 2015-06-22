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

#include "game_mixer.h"

#include "lib/log.h"
#include "lib/std.h"
#include "game_phase.h"
#include "mixer.h"
#include "data.h"

static void start_game_mixer_music(struct state *up)
{
	struct game_mixer_state *self =
		b6_cast_of(up, struct game_mixer_state, up);
	struct game_mixer *mixer = self->mixer;
	struct istream *is;
	int retval;
	mixer->music = 0;
	if (!(is = get_data(mixer->music_data))) {
		log_w(_s("cannot get background music stream"));
		return;
	}
	retval = mixer->mixer->ops->load_music_from_stream(mixer->mixer, is);
	if (!retval) {
		mixer->music = 1;
		play_music(mixer->mixer);
	} else
		logf_w("could not load background music (%d)", retval);
	put_data(mixer->music_data, is);
}

static void stop_game_mixer_music(struct state *up)
{
	struct game_mixer_state *self =
		b6_cast_of(up, struct game_mixer_state, up);
	struct game_mixer *mixer = self->mixer;
	if (!mixer->music)
		return;
	mixer->music = 0;
	stop_music(mixer->mixer);
	unload_music(mixer->mixer);
}

static void update_game_mixer_music(struct state *up)
{
	struct game_mixer_state *self =
		b6_cast_of(up, struct game_mixer_state, up);
	set_music_pos(self->mixer->mixer, self->pos);
}

static void cancel_game_mixer_music(struct state *up)
{
	struct game_mixer_state *self =
		b6_cast_of(up, struct game_mixer_state, up);
	dequeue_state(up, &self->mixer->state_queue);
}

static struct game_mixer *to_game_mixer(struct game_observer *observer)
{
	return b6_cast_of(observer, struct game_mixer, game_observer);
}

static void game_mixer_on_level_enter(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	enqueue_state(&self->normal_music.up, &self->state_queue);
}

static void game_mixer_on_level_leave(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	dequeue_state(&self->normal_music.up, &self->state_queue);
}

static void game_mixer_on_level_passed(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	enqueue_state(&self->passed_music.up, &self->state_queue);
	hold_game_for(self->game, &self->hold, 2100000ULL);
}

static void game_mixer_on_level_failed(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	struct state *state;
	play_sample(self->mixer, self->death);
	while ((state = current_state(&self->state_queue))) {
		if (state == &self->normal_music.up)
			break;
		dequeue_state(state, &self->state_queue);
	}
	if (self->game->pacman.lifes >= 0)
		return;
	enqueue_state(&self->byebye_music.up, &self->state_queue);
	hold_game_for(self->game, &self->hold, 2000000ULL);
}

static void game_mixer_on_pacman_diet_begin(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	enqueue_state(&self->diet_music.up, &self->state_queue);
}

static void game_mixer_on_pacman_diet_end(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	dequeue_state(&self->diet_music.up, &self->state_queue);
}

static void game_mixer_on_pacman_slow_begin(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	enqueue_state(&self->slow_music.up, &self->state_queue);
}

static void game_mixer_on_pacman_slow_end(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	dequeue_state(&self->slow_music.up, &self->state_queue);
}

static void game_mixer_on_pacman_fast_begin(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	enqueue_state(&self->fast_music.up, &self->state_queue);
}

static void game_mixer_on_pacman_fast_end(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	dequeue_state(&self->fast_music.up, &self->state_queue);
}

static void game_mixer_on_ghosts_frozen(struct game_observer *observer,
					int state)
{
	struct game_mixer *self = to_game_mixer(observer);
	if (state > 0)
		enqueue_state(&self->frozen_music.up, &self->state_queue);
	else if (state < 0)
		dequeue_state(&self->frozen_music.up, &self->state_queue);
}

static void game_mixer_on_ghost_state_change(struct game_observer *observer,
					     const struct ghost *ghost)
{
	struct game_mixer *self = to_game_mixer(observer);
	if (get_ghost_state(ghost) == GHOST_ZOMBIE)
		play_sample(self->mixer, self->ghost_eaten);
}

static void game_mixer_on_extra_life(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	play_sample(self->mixer, self->life);
}

static void game_mixer_on_teleport(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	play_sample(self->mixer, self->teleport);
}

static void game_mixer_on_super_pacgum(struct game_observer *observer,
				       int state)
{
	struct game_mixer *self = to_game_mixer(observer);
	if (state > 0) {
		play_sample(self->mixer, self->afraid);
		enqueue_state(&self->afraid_music.up, &self->state_queue);
	} else if (state < 0) {
		play_sample(self->mixer, self->hunter);
		dequeue_state(&self->afraid_music.up, &self->state_queue);
	}
}

static void game_mixer_on_shield_pickup(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	play_sample(self->mixer, self->joker);
}

static void game_mixer_on_shield_change(struct game_observer *observer,
					int state)
{
	struct game_mixer *self = to_game_mixer(observer);
	if (state > 0)
		play_sample(self->mixer, self->shield);
	else if (state < 0)
		play_sample(self->mixer, self->shield);
}

static void game_mixer_on_pickup(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	play_sample(self->mixer, self->bonus);
}

static void game_mixer_on_jewel_pickup(struct game_observer *observer, int n)
{
	game_mixer_on_pickup(observer);
}

static void game_mixer_on_booster_full(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	play_sample(self->mixer, self->honk);
}

static void game_mixer_on_booster_reload(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	play_sample(self->mixer, self->reload);
}

static void game_mixer_on_booster_charge(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	try_play_sample(self->mixer, self->booster);
}

static void game_mixer_on_wipeout(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	play_sample(self->mixer, self->tranquility);
}

static void game_mixer_on_casino_update(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	try_play_sample(self->mixer, self->roll);
}

static void game_mixer_on_casino_finish(struct game_observer *observer)
{
	struct game_mixer *self = to_game_mixer(observer);
	stop_sample(self->mixer, self->roll);
}

static void do_load_sample(struct mixer *mixer, const char *skin,
			   const char *path, struct sample **sample,
			   const char *name)
{
	struct data_entry *data;
	struct istream *istream;
	*sample = NULL;
	if (!(data = lookup_data(skin, audio_data_type, path))) {
		log_w(_s("could not find sample "), _s(name));
		return;
	}
	if (!(istream = get_data(data))) {
		log_w(_s("out of memory when reading sample "), _s(name));
		return;
	}
	if (!(*sample = mixer->ops->load_sample_from_stream(mixer, istream)))
		log_w(_s("could not read sample "), _s(name));
	put_data(data, istream);
}

#define LOAD_SAMPLE(self, sample, skin, path) \
	do_load_sample(self->mixer, skin, path, &self->sample, #sample)

#define KILL_SAMPLE(sample) \
	do { if (sample) destroy_sample(sample); } while (0)

static void reset_game_mixer_state(struct game_mixer_state *self,
				   const struct state_ops *ops,
				   struct game_mixer *mixer, int pos)
{
	reset_state(&self->up, ops);
	self->mixer = mixer;
	self->pos = pos;
}

static int is_greedy(const char *skin_id)
{
	return skin_id[0] == 'g' && skin_id[1] == 'r' && skin_id[2] == 'e' &&
		skin_id[3] == 'e' && skin_id[4] == 'd' && skin_id[5] == 'y' &&
		skin_id[6] == '\0';
}

int initialize_game_mixer(struct game_mixer *self, struct game *game,
			  const char *skin_name, struct mixer *mixer)
{
	static const struct game_observer_ops ops = {
		.on_level_enter = game_mixer_on_level_enter,
		.on_level_leave = game_mixer_on_level_leave,
		.on_level_passed = game_mixer_on_level_passed,
		.on_level_failed = game_mixer_on_level_failed,
		.on_pacman_diet_begin = game_mixer_on_pacman_diet_begin,
		.on_pacman_diet_end = game_mixer_on_pacman_diet_end,
		.on_fast_pacman_on = game_mixer_on_pacman_fast_begin,
		.on_fast_pacman_off = game_mixer_on_pacman_fast_end,
		.on_slow_pacman_on = game_mixer_on_pacman_slow_begin,
		.on_slow_pacman_off = game_mixer_on_pacman_slow_end,
		.on_slow_ghosts_on = game_mixer_on_pickup,
		.on_fast_ghosts_on = game_mixer_on_pickup,
		.on_x2_on = game_mixer_on_pickup,
		.on_zzz = game_mixer_on_ghosts_frozen,
		.on_super_pacgum = game_mixer_on_super_pacgum,
		.on_shield_change = game_mixer_on_shield_change,
		.on_shield_pickup = game_mixer_on_shield_pickup,
		.on_banquet_on = game_mixer_on_pickup,
		.on_extra_life = game_mixer_on_extra_life,
		.on_teleport = game_mixer_on_teleport,
		.on_ghost_state_change = game_mixer_on_ghost_state_change,
		.on_jewel_pickup = game_mixer_on_jewel_pickup,
		.on_booster_charge = game_mixer_on_booster_charge,
		.on_booster_small_reload = game_mixer_on_booster_reload,
		.on_booster_large_reload = game_mixer_on_booster_reload,
		.on_booster_full = game_mixer_on_booster_full,
		.on_wipeout = game_mixer_on_wipeout,
		.on_casino_update = game_mixer_on_casino_update,
		.on_casino_finish = game_mixer_on_casino_finish,
	};
	static const struct state_ops primary_state_ops = {
		.on_enqueue = start_game_mixer_music,
		.on_dequeue = stop_game_mixer_music,
		.on_promote = update_game_mixer_music,
	};
	static const struct state_ops secondary_state_ops = {
		.on_enqueue = update_game_mixer_music,
		.on_demote = cancel_game_mixer_music,
	};
	static const struct state_ops state_no_ops = {};
	const struct state_ops *state_ops;
	self->game = game;
	self->mixer = mixer;
	self->music = 0;
	if (!(self->music_data = lookup_data(skin_name, audio_data_type,
					     GAME_MUSIC_DATA_ID)))
		log_w(_s("cannot find background music for "), _s(skin_name));
	LOAD_SAMPLE(self, bonus, skin_name, GAME_SOUND_BONUS_DATA_ID);
	LOAD_SAMPLE(self, joker, skin_name, GAME_SOUND_JOKER_DATA_ID);
	LOAD_SAMPLE(self, hunter, skin_name, GAME_SOUND_RETURN_DATA_ID);
	LOAD_SAMPLE(self, afraid, skin_name, GAME_SOUND_GHOST_AFRAID_DATA_ID);
	LOAD_SAMPLE(self, ghost_eaten, skin_name,
		    GAME_SOUND_GHOST_DEATH_DATA_ID);
	LOAD_SAMPLE(self, life, skin_name, GAME_SOUND_LIFE_DATA_ID);
	LOAD_SAMPLE(self, death, skin_name, GAME_SOUND_PACMAN_DEATH_DATA_ID);
	LOAD_SAMPLE(self, teleport, skin_name, GAME_SOUND_TELEPORT_DATA_ID);
	LOAD_SAMPLE(self, shield, skin_name, GAME_SOUND_SHIELD_DATA_ID);
	LOAD_SAMPLE(self, reload, skin_name, GAME_SOUND_BOOSTER_RELOAD_DATA_ID);
	LOAD_SAMPLE(self, booster, skin_name, GAME_SOUND_BOOSTER_DRAIN_DATA_ID);
	LOAD_SAMPLE(self, honk, skin_name, GAME_SOUND_BOOSTER_FULL_DATA_ID);
	LOAD_SAMPLE(self, roll, skin_name, GAME_SOUND_CASINO_DATA_ID);
	LOAD_SAMPLE(self, tranquility, skin_name, GAME_SOUND_WIPEOUT_DATA_ID);
	if (self->music_data && is_greedy(skin_name))
		state_ops = &secondary_state_ops;
	else
		state_ops = &state_no_ops;
	reset_state_queue(&self->state_queue);
	reset_game_mixer_state(&self->normal_music, self->music_data ?
			       &primary_state_ops : &state_no_ops, self, 0);
	reset_game_mixer_state(&self->passed_music, state_ops, self, 0x12);
	reset_game_mixer_state(&self->frozen_music, state_ops, self, 0x28);
	reset_game_mixer_state(&self->afraid_music, state_ops, self, 0x21);
	reset_game_mixer_state(&self->byebye_music, state_ops, self, 0x1f);
	reset_game_mixer_state(&self->diet_music, state_ops, self, 0x13);
	reset_game_mixer_state(&self->slow_music, state_ops, self, 0x17);
	reset_game_mixer_state(&self->fast_music, state_ops, self, 0x19);
	reset_game_mixer_state(&self->x2_music, state_ops, self, 0x20);
	reset_game_mixer_state(&self->shield_music, state_ops, self, 0x15);
	b6_reset_event(&self->hold.event, NULL);
	setup_game_observer(&self->game_observer, &ops);
	add_game_observer(game, &self->game_observer);
	return 0;
}

void finalize_game_mixer(struct game_mixer *self)
{
	del_game_observer(&self->game_observer);
	if (self->music)
		unload_music(self->mixer);
	KILL_SAMPLE(self->teleport);
	KILL_SAMPLE(self->death);
	KILL_SAMPLE(self->ghost_eaten);
	KILL_SAMPLE(self->afraid);
	KILL_SAMPLE(self->hunter);
	KILL_SAMPLE(self->joker);
	KILL_SAMPLE(self->bonus);
	KILL_SAMPLE(self->shield);
	KILL_SAMPLE(self->reload);
	KILL_SAMPLE(self->booster);
	KILL_SAMPLE(self->honk);
	KILL_SAMPLE(self->tranquility);
}
