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

static struct lang en = {
	.menu = {
		.play = "Play",
		.options = "Options",
		.hof = "Hiscores",
		.credits = "Credits",
		.quit = "Quit",
		.game_options = "Game options",
		.video_options = "Video options",
		.lang = "Language: English",
		.fullscreen_on = "Mode: fullscreen",
		.fullscreen_off = "Mode: windowed",
		.vsync_on = "Vertical sync: on",
		.vsync_off = "Vertical sync: off",
		.apply = "Apply",
		.back = "Back",
		.game = "Levels: ",
		.shuffle_on = "Order: random",
		.shuffle_off = "Order: normal",
		.mode_slow = "Speed: slow",
		.mode_fast = "Speed: fast",
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
register_lang(en);

const struct lang *__lang_default = &en;

static struct lang fr = {
	.menu = {
		.play = "Jouer",
		.options = "Options",
		.hof = "Meilleurs scores",
		.credits = "Credits",
		.quit = "Quitter",
		.game_options = "Options de jeu",
		.video_options = "Options videos",
		.lang = "Langue: Francais",
		.fullscreen_on = "Mode: plein ecran",
		.fullscreen_off = "Mode: fenetre",
		.vsync_on = "Synchro verticale: on",
		.vsync_off = "Synchro verticale: off",
		.apply = "Appliquer",
		.back = "Retour",
		.game = "Niveaux: ",
		.shuffle_on =  "Ordre: hasard",
		.shuffle_off = "Ordre: normal",
		.mode_slow = "Vitesse: lente ",
		.mode_fast = "Vitesse: rapide",
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
register_lang(fr);
