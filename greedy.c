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

#include <stdio.h>
#include <stdlib.h>
#include <b6/clock.h>
#include <b6/flags.h>
#include "core/console.h"
#include "core/mixer.h"
#include "core/lang.h"
#include "core/engine.h"
#include "core/game.h"
#include "core/renderer.h"
#include "lib/init.h"
#include "lib/log.h"
#include "lib/rng.h"

#define __STRINGIFY(s) #s
#define STRINGIFY(s) __STRINGIFY(s)

#ifdef _WIN32
#include <windows.h>
static void BoostPrio(void)
{
	if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
		log_e("Setting priority class failed: %d", GetLastError());
}
#else
static void BoostPrio(void) {}
#endif /* _WIN32 */
static void RevertPrio(void) {}

#ifdef _WIN32
const char *clock_name = "win32";
#else
const char *clock_name = "linux";
#endif
b6_flag_named(clock_name, string, "clock");

const char *console_name = "sdl/gl";
b6_flag_named(console_name, string, "console");

const char *lang = "en";
b6_flag(lang, string);

static const char *mode = "fast";
b6_flag(mode, string);

static const char *game = "greedy";
b6_flag(game, string);

static const char *log_flag = NULL;
b6_flag_named(log_flag, string, "log");

int main(int argc, char *argv[])
{
	int retval = EXIT_FAILURE;
	struct console *console = NULL;
	struct mixer *mixer = NULL;
	struct b6_named_clock *clock_source = NULL;
	const struct game_config *game_config;
	struct engine engine;
	puts("Open Greedy - Copyright (C) 2014 Arnaud TROEL");
	puts("This program comes with ABSOLUTELY NO WARRANTY.");
	puts("This is free software, and you are welcome to redistribute it");
	puts("under certain conditions; see COPYING file for details.");
#ifdef VERSION
	fputs("Version: " STRINGIFY(VERSION) "\n", stderr);
#endif
#ifdef BUILD
	fputs("Build: " STRINGIFY(BUILD) "\n", stderr);
#endif
	initialize_log();
	b6_parse_command_line_flags(argc, argv, 0);
	if (log_flag)
		switch (*log_flag) {
		case 'D': case 'd': log_level = LOG_DEBUG; break;
		case 'I': case 'i': log_level = LOG_INFO; break;
		case 'W': case 'w': log_level = LOG_WARNING; break;
		case 'E': case 'e': log_level = LOG_ERROR; break;
		case 'P': case 'p': log_level = LOG_PANIC; break;
		}
	if (!(clock_source = b6_lookup_named_clock(clock_name)))
		log_e("cannot find clock %s", clock_name);
	if (!(clock_source = b6_get_default_named_clock()))
		log_p("cannot find a default clock");
	log_clock = clock_source->clock;
	log_i("using %s clock source", clock_source->entry.name);
	init_all();
	if (!(game_config = lookup_game_config(mode)))
		log_p("unknown mode: %s", mode);
	if (!(console = lookup_console(console_name)))
		log_p("unknown console: %s", console_name);
	log_i("using %s console", console_name);
	BoostPrio();
	mixer = lookup_mixer("sdl");
	if (open_mixer(mixer))
		goto bail_out;
	setup_engine(&engine, clock_source->clock, console, mixer,
		     lookup_lang(lang), game, game_config);
	reset_random_number_generator(b6_get_clock_time(engine.clock));
	run_engine(&engine);
	retval = EXIT_SUCCESS;
bail_out:
	if (mixer)
		close_mixer(mixer);
	RevertPrio();
	exit_all();
	return retval;
}
