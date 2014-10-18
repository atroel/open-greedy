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

#include "lang.h"

B6_REGISTRY_DEFINE(__lang_registry);

const struct lang __lang_default = {
	.menu = {
		.play = "Play",
		.hof = "Hiscores",
		.credits = "Credits",
		.quit = "Quit",
		.mode_slow = "Mode: slow",
		.mode_fast = "Mode: fast",
	},
	.game = {
		.get_ready      = "GET READY!",
		.paused         = "PAUSED",
		.try_again      = "TRY AGAIN!",
		.game_over      = "GAME OVER!",
		.happy_hour     = "HAPPY HOUR!",
		.bullet_proof   = "&BULLET PROOF&",
		.freeze         = "FREEZE!",
		.eat_the_ghosts = "EAT THE GHOSTS",
		.extra_life     = "EXTRA LIFE",
		.empty_shield   = "EMPTY SHIELD",
		.shield_pickup  = "SHIFT=SHIELD",
		.level_complete = "LEVEL COMPLETE",
		.next_level     = "NEXT LEVEL",
		.previous_level = "PREVIOUS LEVEL",
		.slow_ghosts    = "SLOW GHOSTS",
		.fast_ghosts    = "FAST GHOSTS",
		.speed_slow     = "SPEED: SLOW",
		.speed_fast     = "SPEED: FAST",
		.single_booster = "SINGLE BOOSTER",
		.double_booster = "DOUBLE BOOSTER",
		.booster_full   = "&BOOSTER FULL&",
		.booster_empty  = "BOOSTER EMPTY",
		.wiped_out      = "WIPED OUT!",
		.bonus_200      = "BONUS: 200",
		.dieting        = "DIETING",
		.casino         = " CASINO:      ",
	},
	.credits = {
		.lines = {
			"",
			"",
			"",
			"- Open Greedy -",
			"",
			"Copyright (C) 2014 - Arnaud TROEL",
			"",
			"",
			"This program is free software: you are free to",
			"redistribute it and/or modify it under the terms of",
			"the GNU General Public License version 2 or later.",
			"",
			"",
			"",
			"This code is a rewrite from scratch of",
			"\177 Greedy XP \177",
			"(http://www.edromel.com)",
			"",
			"Code       Marc RADERMACHER",
			"Graphics    Patrick LACHAUX",
			"Music    Christophe RESIGNE",
			"",
			"",
			"",
			"This program relies on the following libraries:",
			"",
			"SDL, SDL_mixer, OpenGL, libxmp-lite and zlib",
			"",
			"",
			""
		},
	},
};

register_lang(fr) = {
	.menu = {
		.play = "Jouer",
		.hof = "Meilleurs scores",
		.credits = "Credits",
		.quit = "Quitter",
		.mode_slow = "Mode: lent",
		.mode_fast = "Mode: rapide",
	},
	.game = {
		.get_ready      = "PRET?",
		.paused         = "PAUSE",
		.try_again      = "RECOMMENCE!",
		.game_over      = "JEU TERMINE!",
		.happy_hour     = "HAPPY HOUR!",
		.bullet_proof   = "&ARMURE&",
		.freeze         = "MAINS EN L'AIR",
		.eat_the_ghosts = "A TABLE!",
		.extra_life     = "NOUVELLE VIE",
		.empty_shield   = "PAS D'ARMURE",
		.shield_pickup  = "SHIFT=ARMURE",
		.level_complete = "NIVEAU TERMINE",
		.next_level     = "NIVEAU SUIV.",
		.previous_level = "NIVEAU PREC.",
		.slow_ghosts    = "RELAX!",
		.fast_ghosts    = "LE STRESS!",
		.speed_slow     = "EN MODE TORTUE",
		.speed_fast     = "EN MODE LIEVRE",
		.single_booster = "BOOSTER SIMPLE",
		.double_booster = "BOOSTER DOUBLE",
		.booster_full   = "BOOSTER PLEIN",
		.booster_empty  = "BOOSTER VIDE",
		.wiped_out      = "ON DEGAGE!",
		.bonus_200      = "BONUS: 200",
		.dieting        = "AU REGIME!",
		.casino         = " CASINO:      ",
	},
	.credits = {
		.lines = {
			"",
			"",
			"",
			"- Open Greedy -",
			"",
			"Copyright (C) 2014 - Arnaud TROEL",
			"",
			"",
			"Ce programme est un logiciel libre: vous pouvez le",
			"modifier ou le redistribuer selon les termes de la",
			"GNU General Public License version 2 ou suivante. ",
			"",
			"",
			"",
			"Ce code est une reecriture complete de",
			"\177 Greedy XP \177",
			"(http://www.edromel.com)",
			"",
			"Code        Marc RADERMACHER",
			"Graphisme    Patrick LACHAUX",
			"Musique   Christophe RESIGNE",
			"",
			"",
			"",
			"Ce programme repose sur les bibliotheques suivantes:",
			"",
			"SDL, SDL_mixer, OpenGL, libxmp-lite et zlib",
			"",
			"",
			""
		},
	},
};
