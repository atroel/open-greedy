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

#define GAME_FONT_DATA_ID "game.font"
#define GAME_PANEL_DATA_ID "game.panel"
#define GAME_BOOSTER_DATA_ID "game.booster"
#define GAME_CASINO_DATA_ID "game.casino"
#define GAME_JEWEL_DATA_ID(n) "game.jewel." #n
#define GAME_LIFE_ICON_DATA_ID "game.life_icon"
#define GAME_SHIELD_ICON_DATA_ID "game.shield_icon"
#define GAME_LAYOUT_DATA_ID "game.layout"
#define GAME_BOTTOM_DATA_ID(where) "game.bottom." where
#define GAME_PACMAN_DATA_ID(mode, dir) "game.pacman." mode "." dir
#define GAME_GHOST_DATA_ID(n, mode, dir) "game.ghost." #n "." mode "." dir
#define GAME_POINTS_DATA_ID(n) "game.points." #n
#define GAME_PACGUM_DATA_ID "game.pacgum"
#define GAME_SUPER_PACGUM_DATA_ID "game.super_pacgum"
#define GAME_PACMAN_WINS_DATA_ID "game.pacman.wins"
#define GAME_PACMAN_LOSES_DATA_ID "game.pacman.loses"

#define GAME_INFO_JEWEL1_DATA_ID "info.jewel.1"
#define GAME_INFO_JEWEL2_DATA_ID "info.jewel.2"
#define GAME_INFO_JEWEL3_DATA_ID "info.jewel.3"
#define GAME_INFO_JEWEL4_DATA_ID "info.jewel.4"
#define GAME_INFO_JEWEL5_DATA_ID "info.jewel.5"
#define GAME_INFO_JEWEL6_DATA_ID "info.jewel.6"
#define GAME_INFO_JEWEL7_DATA_ID "info.jewel.7"
#define GAME_INFO_FORWARD_DATA_ID "info.level.forward"
#define GAME_INFO_REWIND_DATA_ID "info.level.rewind"
#define GAME_INFO_GHOSTS_SLOW_DATA_ID "info.ghosts.slow"
#define GAME_INFO_GHOSTS_FAST_DATA_ID "info.ghosts.fast"
#define GAME_INFO_WIPEOUT_DATA_ID "info.wipeout"
#define GAME_INFO_LOCKDOWN_DATA_ID "info.lockdown"
#define GAME_INFO_SUPER_PACGUM_DATA_ID "info.super_pacgum"
#define GAME_INFO_BANQUET_DATA_ID "info.banquet"
#define GAME_INFO_PACMAN_SLOW_DATA_ID "info.pacman.slow"
#define GAME_INFO_PACMAN_FAST_DATA_ID "info.pacman.fast"
#define GAME_INFO_SHIELD_DATA_ID "info.shield"
#define GAME_INFO_CASINO_DATA_ID "info.casino"
#define GAME_INFO_LIFE_DATA_ID "info.life"
#define GAME_INFO_DEATH_DATA_ID "info.death"
#define GAME_INFO_DIET_DATA_ID "info.diet"
#define GAME_INFO_SINGLE_BOOSTER_DATA_ID "info.booster.x1"
#define GAME_INFO_DOUBLE_BOOSTER_DATA_ID "info.booster.x2"
#define GAME_INFO_X2_DATA_ID "info.x2"
#define GAME_INFO_COMPLETE_DATA_ID "info.level.complete"
#define GAME_INFO_INVINCIBLE_DATA_ID "info.invincible"
#define GAME_INFO_EMPTY_DATA_ID "info.empty"

#define GAME_BONUS_JEWELS_DATA_ID "bonus.jewels"
#define GAME_BONUS_JEWEL1_DATA_ID "bonus.jewel.1"
#define GAME_BONUS_JEWEL2_DATA_ID "bonus.jewel.2"
#define GAME_BONUS_JEWEL3_DATA_ID "bonus.jewel.3"
#define GAME_BONUS_JEWEL4_DATA_ID "bonus.jewel.4"
#define GAME_BONUS_JEWEL5_DATA_ID "bonus.jewel.5"
#define GAME_BONUS_JEWEL6_DATA_ID "bonus.jewel.6"
#define GAME_BONUS_JEWEL7_DATA_ID "bonus.jewel.7"
#define GAME_BONUS_SINGLE_BOOSTER_DATA_ID "bonus.booster_x1"
#define GAME_BONUS_DOUBLE_BOOSTER_DATA_ID "bonus.booster_x2"
#define GAME_BONUS_SUPER_PACGUM_DATA_ID "bonus.super_pacgum"
#define GAME_BONUS_LIFE_DATA_ID "bonus.life"
#define GAME_BONUS_PACMAN_SLOW_DATA_ID "bonus.pacman.slow"
#define GAME_BONUS_PACMAN_FAST_DATA_ID "bonus.pacman.fast"
#define GAME_BONUS_SHIELD_DATA_ID "bonus.shield"
#define GAME_BONUS_DIET_DATA_ID "bonus.diet"
#define GAME_BONUS_DEATH_DATA_ID "bonus.death"
#define GAME_BONUS_GHOSTS_BANQUET_DATA_ID "bonus.banquet"
#define GAME_BONUS_GHOSTS_WIPEOUT_DATA_ID "bonus.wipeout"
#define GAME_BONUS_GHOSTS_SLOW_DATA_ID "bonus.ghosts.slow"
#define GAME_BONUS_GHOSTS_FAST_DATA_ID "bonus.ghosts.fast"
#define GAME_BONUS_LOCKDOWN_DATA_ID "bonus.lockdown"
#define GAME_BONUS_X2_DATA_ID "bonus.x2"
#define GAME_BONUS_REWIND_DATA_ID "bonus.level.rewind"
#define GAME_BONUS_FORWARD_DATA_ID "bonus.level.forward"
#define GAME_BONUS_CASINO_DATA_ID "bonus.casino"
#define GAME_BONUS_SURPRISE_DATA_ID "bonus.surprise"

#define GAME_SOUND_GHOST_AFRAID_DATA_ID "game.afraid"
#define GAME_SOUND_BONUS_DATA_ID "game.bonus"
#define GAME_SOUND_BOOSTER_FULL_DATA_ID "game.booster.full"
#define GAME_SOUND_BOOSTER_DRAIN_DATA_ID "game.booster.drain"
#define GAME_SOUND_BOOSTER_RELOAD_DATA_ID "game.booster.reload"
#define GAME_SOUND_GHOST_DEATH_DATA_ID "game.ghost.death"
#define GAME_SOUND_PACMAN_DEATH_DATA_ID "game.pacman.death"
#define GAME_SOUND_SHIELD_DATA_ID "game.shield"
#define GAME_SOUND_JOKER_DATA_ID "game.joker"
#define GAME_SOUND_LIFE_DATA_ID "game.life"
#define GAME_SOUND_RETURN_DATA_ID "game.return"
#define GAME_SOUND_RING_DATA_ID "game.ring"
#define GAME_SOUND_CASINO_DATA_ID "game.casino"
#define GAME_SOUND_TELEPORT_DATA_ID "game.teleport"
#define GAME_SOUND_WIPEOUT_DATA_ID "game.wipeout"

#define GAME_MUSIC_DATA_ID "game.music"
