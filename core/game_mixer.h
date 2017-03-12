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

#ifndef GAME_MIXER_H
#define GAME_MIXER_H

#include "lib/state.h"
#include "game.h"

struct game_mixer_state {
	struct state up;
	struct game_mixer *mixer;
	int pos;
};

struct game_mixer {
	struct game_observer game_observer;
	struct game *game;
	struct mixer *mixer;
	enum ghost_state ghost_state[4];
	struct sample *hunter;
	struct sample *afraid;
	struct sample *ghost_eaten;
	struct sample *life;
	struct sample *death;
	struct sample *teleport;
	struct sample *bonus;
	struct sample *joker;
	struct sample *shield;
	struct sample *reload;
	struct sample *booster;
	struct sample *honk;
	struct sample *roll;
	struct sample *tranquility;
	struct data_entry *music_data;
	int music;
	struct game_event hold;
	struct state_queue state_queue;
	struct game_mixer_state normal_music;
	struct game_mixer_state passed_music;
	struct game_mixer_state frozen_music;
	struct game_mixer_state afraid_music;
	struct game_mixer_state byebye_music;
	struct game_mixer_state diet_music;
	struct game_mixer_state slow_music;
	struct game_mixer_state fast_music;
	struct game_mixer_state x2_music;
	struct game_mixer_state shield_music;
};

extern int initialize_game_mixer(struct game_mixer *self, struct game *game,
				 const char *skin_name, struct mixer *mixer);

extern void finalize_game_mixer(struct game_mixer *self);

#endif /* GAME_MIXER_H */
