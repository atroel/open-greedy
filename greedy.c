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

#include <stdio.h>
#include <stdlib.h>
#include <b6/clock.h>
#include <b6/cmdline.h>
#include <b6/json.h>
#include <b6/utf8.h>

#include "core/console.h"
#include "core/mixer.h"
#include "core/engine.h"
#include "core/json.h"
#include "core/game.h"
#include "core/preferences.h"
#include "core/renderer.h"
#include "lib/embedded.h"
#include "lib/init.h"
#include "lib/log.h"
#include "lib/rng.h"

extern void install_crash_pad(void);

const char *clock_name = PLATFORM;
b6_flag_named(clock_name, string, "clock");

const char *console_name = "sdl/gl";
b6_flag_named(console_name, string, "console");

static const char *log_flag = NULL;
b6_flag_named(log_flag, string, "log");

static int fs = -1;
b6_flag(fs, bool);

static int vs = -1;
b6_flag(vs, bool);

static int shuffle = -1;
b6_flag(shuffle, bool);

static const char *game = NULL;
b6_flag(game, string);

static const char *mode = NULL;
b6_flag(mode, string);

static const char *lang = NULL;
b6_flag(lang, string);

/* ascii to lev */
static int a2l(struct b6_cmd *b6_cmd, int argc, char *argv[])
{
	struct layout layout;
	struct ifstream ifs;
	struct ofstream ofs;
	int retval;
	initialize_ifstream_with_fp(&ifs, stdin, 0);
	retval = parse_layout(&layout, &ifs.istream);
	finalize_ifstream(&ifs);
	if (retval)
		return EXIT_FAILURE;
	initialize_ofstream_with_fp(&ofs, stdout, 0);
	retval = serialize_layout(&layout, &ofs.ostream);
	finalize_ofstream(&ofs);
	if (retval)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
b6_cmd(a2l);

/* lev to ascii */
static int l2a(struct b6_cmd *cmd, int argc, char *argv[])
{
	struct layout layout;
	struct ifstream ifs;
	struct ofstream ofs;
	int retval;
	initialize_ifstream_with_fp(&ifs, stdin, 0);
	retval = unserialize_layout(&layout, &ifs.istream);
	finalize_ifstream(&ifs);
	if (retval)
		return EXIT_FAILURE;
	initialize_ofstream_with_fp(&ofs, stdout, 0);
	retval = print_layout(&layout, &ofs.ostream);
	finalize_ofstream(&ofs);
	if (retval)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
b6_cmd(l2a);

/* data to z */
static int d2z(struct b6_cmd *cmd, int argc, char *argv[])
{
	unsigned char zbuf[4096];
	struct ifstream ifs;
	struct ofstream ofs;
	struct ozstream ozs;
	int retval = EXIT_FAILURE;
	initialize_ifstream_with_fp(&ifs, stdin, 0);
	initialize_ofstream_with_fp(&ofs, stdout, 0);
	if (!initialize_ozstream(&ozs, &ofs.ostream, zbuf, sizeof(zbuf)) &&
	    pipe_streams(&ifs.istream, &ozs.up) > 0)
		retval = EXIT_SUCCESS;
	finalize_ozstream(&ozs);
	finalize_ofstream(&ofs);
	finalize_ifstream(&ifs);
	return retval;
}
b6_cmd(d2z);

/* z to data */
static int z2d(struct b6_cmd *cmd, int argc, char *argv[])
{
	unsigned char zbuf[4096];
	struct ifstream ifs;
	struct ofstream ofs;
	struct izstream izs;
	int retval = EXIT_FAILURE;
	initialize_ofstream_with_fp(&ofs, stdout, 0);
	initialize_ifstream_with_fp(&ifs, stdin, 0);
	if (!initialize_izstream(&izs, &ifs.istream, zbuf, sizeof(zbuf)) &&
	    pipe_streams(&izs.up, &ofs.ostream) > 0)
		retval = EXIT_SUCCESS;
	finalize_izstream(&izs);
	finalize_ifstream(&ifs);
	finalize_ofstream(&ofs);
	return retval;
}
b6_cmd(z2d);

static struct b6_json_object *get_embedded_lang(struct b6_json *json)
{
	struct b6_json_object *lang = NULL;
	struct embedded *embedded = NULL;
	struct izbstream izbs;
	struct json_istream jis;
	struct b6_json_parser_info info;
	enum b6_json_error error;
	if (!(embedded = lookup_embedded(B6_UTF8("lang.json"))))
		return NULL;
	if (initialize_izbstream(&izbs, embedded->buf, embedded->len))
		return NULL;
	setup_json_istream(&jis, izbstream_as_istream(&izbs));
	info.row = info.col = 0;
	b6_json_reset_parser_info(&info);
	if (!(lang = b6_json_new_object(json)))
		log_e(_s("out of memory"));
	else if ((error = b6_json_parse_object(lang, &jis.up, &info))) {
		logf_e("json parsing failed (%d): row=%d col=%d", error,
		      info.row, info.col);
		b6_json_unref_value(&lang->up);
		lang = NULL;
	}
	finalize_izbstream(&izbs);
	return lang;
}

static int greedy(struct b6_clock *clock)
{
	int retval = EXIT_FAILURE;
	struct console *console = NULL;
	struct mixer *mixer = NULL;
	struct b6_json *json = NULL;
	struct b6_json_object *languages = NULL;
	struct preferences preferences;
	struct engine engine;
	struct b6_utf8 utf8;
	puts("Open Greedy - Copyright (C) 2014-2017 Arnaud TROEL");
	puts("This program comes with ABSOLUTELY NO WARRANTY.");
	puts("This is free software, and you are welcome to redistribute it");
	puts("under certain conditions; see COPYING file for details.");
	fprintf(stderr, "Build: %s-%s-v%s\n", PLATFORM, BUILD, VERSION);
	init_all();
	b6_utf8_from_ascii(&utf8, console_name);
	if (!(console = lookup_console(&utf8))) {
		log_e(_s("unknown console: "), _s(console_name));
		goto bail_out;
	}
	log_i(_s("using "), _s(console_name), _s(" console"));
	mixer = lookup_mixer(B6_UTF8("sdl"));
	if (open_mixer(mixer))
		goto bail_out;
	if (!(json = get_json()))
		goto bail_out;
	if (!(languages = get_embedded_lang(json)))
		goto bail_out;
	reset_random_number_generator(b6_get_clock_time(clock));
	if (initialize_pref(&preferences, json, "prefs.json.z"))
		goto bail_out;
	load_pref(&preferences);
	if (vs >= 0)
		set_pref_vsync(&preferences, vs);
	if (fs >= 0)
		set_pref_fullscreen(&preferences, fs);
	if (shuffle >= 0)
		set_pref_shuffle(&preferences, shuffle);
	if (mode) {
		if (lookup_game_config(b6_utf8_from_ascii(&utf8, mode)))
			set_pref_mode(&preferences, &utf8);
	}
	if (game)
		set_pref_game(&preferences, b6_utf8_from_ascii(&utf8, game));
	if (lang)
		set_pref_lang(&preferences, b6_utf8_from_ascii(&utf8, lang));
	if (!initialize_engine(&engine, clock, console, mixer, &preferences,
			       languages)) {
		run_engine(&engine);
		finalize_engine(&engine);
		retval = EXIT_SUCCESS;
	}
	save_pref(&preferences);
	finalize_pref(&preferences);
bail_out:
	if (languages)
		b6_json_unref_value(&languages->up);
	if (json)
		put_json(json);
	if (mixer)
		close_mixer(mixer);
	exit_all();
	return retval;
}

int main(int argc, char *argv[])
{
	struct b6_cmd *cmd;
	int argn, retval;
	struct b6_named_clock *clock_source = NULL;
	struct b6_utf8 utf8;
	install_crash_pad();
	argn = b6_parse_command_line_flags(argc, argv, 0);
	if (log_flag)
		switch (*log_flag) {
		case 'D': case 'd': log_level = LOG_D; break;
		case 'I': case 'i': log_level = LOG_I; break;
		case 'W': case 'w': log_level = LOG_W; break;
		case 'E': case 'e': log_level = LOG_E; break;
		case 'P': case 'p': log_level = LOG_P; break;
		}
	if (clock_name) {
		b6_utf8_from_ascii(&utf8, clock_name);
		if (!(clock_source = b6_lookup_named_clock(&utf8)))
			log_e(_s("cannot find clock "), _s(clock_name));
	}
	if (!clock_source && !(clock_source = b6_get_default_named_clock()))
		log_p(_s("cannot find a default clock"));
	set_log_clock(clock_source->clock);
	log_i(_s("using "), _t(clock_source->entry.id), _s(" clock source"));
	if (argn <= 0)
		log_p(_s("error parsing command line"));
	if (argn < argc &&
	    (cmd = b6_lookup_cmd(b6_utf8_from_ascii(&utf8, argv[argn]))))
		retval = b6_exec_cmd(cmd, argc - argn, argv + argn);
	else
		retval = greedy(clock_source->clock);
	log_flush();
	return retval;
}
