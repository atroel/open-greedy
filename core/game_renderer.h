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

#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include "lib/state.h"
#include "fade_io.h"
#include "game.h"
#include "linear.h"
#include "rgba.h"
#include "renderer.h"
#include "toolkit.h"

struct game_renderer_count {
	struct renderer_observer observer;
	struct game_renderer *game_renderer;
	unsigned int value;
	int dirty;
	struct rgba rgba;
	struct rgba icon;
	struct renderer_tile *tile;
};

struct game_renderer_gauge {
	struct renderer_observer observer;
	struct game_renderer *game_renderer;
	struct renderer_tile *tile;
	struct rgba skin;
	struct rgba rgba;
	float value;
	short int notified_width;
	short int rendered_width;
};

struct cartoon {
	unsigned int length;  /* count of images */
	unsigned int period;  /* in microseconds */
	int loop;
	unsigned long long int time;  /* start time in us */
	struct renderer_texture *texture;
	unsigned int index;
};

struct game_renderer_sprite {
	struct game_renderer *game_renderer;
	struct renderer_observer observer;
	const struct cartoon *cartoon;
	struct renderer_tile *tile;
};

struct game_renderer_info {
	struct state up;
	struct game_renderer *game_renderer;
	struct renderer_texture *texture;
	struct renderer_texture *icon_texture;
	unsigned long long int expire;
};

struct game_renderer_casino {
	struct game_renderer_info game_renderer_info;
	struct rgba roll_image;
	unsigned short int roll_width;
	unsigned short int roll_count;
	unsigned int roll_index[3];
	struct rgba rgba;
};

struct game_renderer_popup {
	struct renderer_observer renderer_observer;
	struct renderer *renderer;
	struct renderer_tile *tile;
	struct linear linear;
};

struct game_renderer_jewels {
	struct renderer_observer renderer_observer;
	struct game_renderer *game_renderer;
	struct renderer_texture *textures[7];
	struct renderer_tile *tiles[7];
	struct linear linear;
};

struct b6_json_object;

struct game_renderer {
	struct game_observer game_observer;
	struct renderer_observer renderer_observer;
	struct renderer *renderer;
	const struct b6_clock *clock;
	unsigned long long int time;
	struct game *game;
	const char *skin_id;
	struct game_event hold;

	struct renderer_texture *pacgum_texture;
	struct renderer_texture *super_pacgum_texture;

	struct renderer_tile *playground;

	struct cartoon pacman_cartoons[2][4];
	enum direction pacman_direction;
	int pacman_state;
	struct game_renderer_sprite pacman;

	struct cartoon hunter[4];
	struct cartoon afraid;
	struct cartoon locked;
	struct cartoon zombie;
	struct cartoon wiped_out[4];
	struct game_renderer_sprite ghosts[4];
	int ghosts_state[4];
	short int afraid_state;
	short int locked_state;

	struct cartoon won;
	struct cartoon lost;

	struct cartoon bonus_cartoons[BONUS_COUNT];
	struct game_renderer_sprite bonus;

	struct fixed_font font;

	struct fade_io fade_io;

	unsigned int rendered_score;
	unsigned int notified_score;
	struct renderer_tile *top;
	struct renderer_tile *bottom[2];
	struct toolkit_label score;
	struct toolkit_label level;
	struct game_renderer_count lifes;
	struct game_renderer_count shields;
	struct game_renderer_jewels jewels;
	struct game_renderer_gauge booster;
	struct toolkit_image gums[LEVEL_WIDTH][LEVEL_HEIGHT];
	struct toolkit_image *pacgums;
	struct toolkit_image *super_pacgums;
	struct game_renderer_casino casino;
	struct game_renderer_popup points[9];

	struct renderer_tile *info_tile;
	struct renderer_tile *info_icon_tile;
	struct state_queue info_queue;
	struct game_renderer_info get_ready_info;
	struct game_renderer_info try_again_info;
	struct game_renderer_info game_over_info;
	struct game_renderer_info paused_info;
	struct game_renderer_info happy_hour_info;
	struct game_renderer_info bullet_proof_info;
	struct game_renderer_info freeze_info;
	struct game_renderer_info eat_the_ghosts_info;
	struct game_renderer_info extra_life_info;
	struct game_renderer_info empty_shield_info;
	struct game_renderer_info shift_shield_info;
	struct game_renderer_info level_complete_info;
	struct game_renderer_info next_level_info;
	struct game_renderer_info previous_level_info;
	struct game_renderer_info slow_ghosts_info;
	struct game_renderer_info fast_ghosts_info;
	struct game_renderer_info slow_pacman_info;
	struct game_renderer_info fast_pacman_info;
	struct game_renderer_info single_booster_info;
	struct game_renderer_info double_booster_info;
	struct game_renderer_info full_booster_info;
	struct game_renderer_info empty_booster_info;
	struct game_renderer_info wiped_out_info;
	struct game_renderer_info jewel_info[7];
	struct game_renderer_info diet_info;
};

extern int initialize_game_renderer(struct game_renderer *self,
				    struct renderer *renderer,
				    const struct b6_clock *clock,
				    struct game *game,
				    const char *skin_id,
				    const struct b6_json_object *lang);

extern void finalize_game_renderer(struct game_renderer *self);

#endif /*GAME_RENDERER_H*/
