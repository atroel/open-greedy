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

#include "game_controller.h"

#include "game.h"

static void game_controller_handle_key_pressed(struct game *game,
					       enum controller_key key)
{
	switch (key) {
	case CTRLK_UP:    submit_pacman_move(game, LEVEL_N); break;
	case CTRLK_DOWN:  submit_pacman_move(game, LEVEL_S); break;
	case CTRLK_LEFT:  submit_pacman_move(game, LEVEL_W); break;
	case CTRLK_RIGHT: submit_pacman_move(game, LEVEL_E); break;
	case CTRLK_ESCAPE: abort_game(game); break;
	case CTRLK_p: pause_game(game); break;
	case CTRLK_0: activate_game_bonus(game, JEWELS_BONUS); break;
	case CTRLK_1: activate_game_bonus(game, JEWEL1_BONUS); break;
	case CTRLK_2: activate_game_bonus(game, JEWEL2_BONUS); break;
	case CTRLK_3: activate_game_bonus(game, JEWEL3_BONUS); break;
	case CTRLK_4: activate_game_bonus(game, JEWEL4_BONUS); break;
	case CTRLK_5: activate_game_bonus(game, JEWEL5_BONUS); break;
	case CTRLK_6: activate_game_bonus(game, JEWEL6_BONUS); break;
	case CTRLK_7: activate_game_bonus(game, JEWEL7_BONUS); break;
	case CTRLK_8: activate_game_bonus(game, SINGLE_BOOSTER_BONUS); break;
	case CTRLK_9: activate_game_bonus(game, DOUBLE_BOOSTER_BONUS); break;
	case CTRLK_s: activate_game_bonus(game, SUPER_PACGUM_BONUS); break;
	case CTRLK_v: activate_game_bonus(game, EXTRA_LIFE_BONUS); break;
	case CTRLK_MINUS: activate_game_bonus(game, PACMAN_SLOW_BONUS); break;
	case CTRLK_EQUAL: activate_game_bonus(game, PACMAN_FAST_BONUS); break;
	case CTRLK_i: activate_game_bonus(game, SHIELD_BONUS); break;
	case CTRLK_d: activate_game_bonus(game, DIET_BONUS); break;
	case CTRLK_q: activate_game_bonus(game, SUICIDE_BONUS); break;
	case CTRLK_e: activate_game_bonus(game, GHOSTS_BANQUET_BONUS); break;
	case CTRLK_w: activate_game_bonus(game, GHOSTS_WIPEOUT_BONUS); break;
	case CTRLK_r: activate_game_bonus(game, GHOSTS_SLOW_BONUS); break;
	case CTRLK_t: activate_game_bonus(game, GHOSTS_FAST_BONUS); break;
	case CTRLK_z: activate_game_bonus(game, ZZZ_BONUS); break;
	case CTRLK_x: activate_game_bonus(game, X2_BONUS); break;
	case CTRLK_PERIOD: activate_game_bonus(game, PLUS_BONUS); break;
	case CTRLK_COMMA: activate_game_bonus(game, MINUS_BONUS); break;
	case CTRLK_c: activate_game_bonus(game, CASINO_BONUS); break;
	case CTRLK_LSHIFT:
	case CTRLK_RSHIFT: activate_game_shield(game); break;
	case CTRLK_LCTRL:
	case CTRLK_RCTRL: activate_game_booster(game); break;
	default:
		break;
	}
}

static void game_controller_handle_key_released(struct game *game,
						enum controller_key key)
{
	switch (key) {
	case CTRLK_UP:    cancel_pacman_move(game, LEVEL_N); break;
	case CTRLK_DOWN:  cancel_pacman_move(game, LEVEL_S); break;
	case CTRLK_LEFT:  cancel_pacman_move(game, LEVEL_W); break;
	case CTRLK_RIGHT: cancel_pacman_move(game, LEVEL_E); break;
	case CTRLK_LCTRL:
	case CTRLK_RCTRL: deactivate_game_booster(game); break;
	default:
		break;
	}
}

static void game_controller_on_key_pressed(struct controller_observer *observer,
					   enum controller_key key)
{
	struct game_controller *self =
		b6_cast_of(observer, struct game_controller,
			   controller_observer);
	game_controller_handle_key_pressed(self->game, key);
}

static void game_controller_on_key_released(
	struct controller_observer *observer, enum controller_key key)
{
	struct game_controller *self =
		b6_cast_of(observer, struct game_controller,
			   controller_observer);
	game_controller_handle_key_released(self->game, key);
}

const struct controller_observer_ops __game_controller_observer_ops = {
	.on_key_pressed = game_controller_on_key_pressed,
	.on_key_released = game_controller_on_key_released,
};
