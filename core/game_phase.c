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

#include "lib/init.h"

#include "console.h"
#include "controller.h"
#include "engine.h"
#include "game.h"
#include "game_controller.h"
#include "game_mixer.h"
#include "game_renderer.h"
#include "renderer.h"
#include "data.h"

#include <b6/clock.h>
#include <b6/cmdline.h>
#include <b6/json.h>

struct game_phase {
	struct phase up;
	struct b6_cached_clock clock;
	struct game game;
	struct game_mixer mixer;
	struct game_renderer renderer;
	struct game_controller controller;
};

static unsigned int level = 0;
b6_flag(level, uint);

static const char *game_skin = NULL;
b6_flag(game_skin, string);

static struct game_phase *to_game_phase(struct phase *up)
{
	return b6_cast_of(up, struct game_phase, up);
}

static int game_phase_init(struct phase *up, const struct phase *prev)
{
	struct game_phase *self = to_game_phase(up);
	const char *skin_id = game_skin ? game_skin : get_skin_id();
	struct b6_json_object *lang;
	int retval;
	b6_setup_cached_clock(&self->clock, up->engine->clock);
	if ((retval = initialize_game(&self->game, &self->clock.up,
				      up->engine->game_config,
				      get_engine_layouts(up->engine), level))) {
		log_e("cannot initialize game (%d)", retval);
		goto fail_game;
	}
	lang = b6_json_value_as(get_engine_language(up->engine)->value, object);
	if (!(lang = b6_json_get_object_as(lang, "game", object))) {
		log_e("cannot find game text");
		goto fail_renderer;
	}
	if ((retval = initialize_game_renderer(&self->renderer,
					       get_engine_renderer(up->engine),
					       &self->clock.up, &self->game,
					       skin_id, lang))) {
		log_e("cannot initialize game renderer (%d)", retval);
		goto fail_renderer;
	}
	initialize_game_mixer(&self->mixer, &self->game, skin_id,
			      up->engine->mixer);
	initialize_game_controller(&self->controller, &self->game,
				   get_engine_controller(up->engine));
	up->engine->game_result.score = 0;
	up->engine->game_result.level = 0;
	return 0;
fail_renderer:
	finalize_game(&self->game);
fail_game:
	return -1;
}

static void game_phase_exit(struct phase *up)
{
	struct game_phase *self = to_game_phase(up);
	struct game_result game_result;
	game_result.score = self->game.pacman.score;
	game_result.level = self->game.n - 1;
	set_last_game_result(up->engine, &game_result);
	finalize_game_mixer(&self->mixer);
	finalize_game_renderer(&self->renderer);
	finalize_game_controller(&self->controller);
	finalize_game(&self->game);
}

static void game_phase_suspend(struct phase *up)
{
	pause_game(&to_game_phase(up)->game);
}

static struct phase *game_phase_exec(struct phase *up)
{
	struct game_phase *self = to_game_phase(up);
	b6_sync_cached_clock(&self->clock);
	if (!play_game(&self->game))
		return lookup_phase("hall_of_fame");
	update_game(&self->game);
	return up;
}

static int game_phase_ctor(void)
{
	static const struct phase_ops ops = {
		.init = game_phase_init,
		.exit = game_phase_exit,
		.exec = game_phase_exec,
		.suspend = game_phase_suspend,
	};
	static struct game_phase game_phase;
	return register_phase(&game_phase.up, "game", &ops);
}
register_init(game_phase_ctor);
