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

#include "preferences.h"

#include <b6/json.h>
#include <b6/utf8.h>

#include "lib/io.h"
#include "lib/log.h"
#include "lib/std.h"

#include "env.h"
#include "json.h"

#define PREF_VIDEO_OPTIONS "video_options"
#define PREF_GAME_OPTIONS "game_options"
#define PREF_UI_OPTIONS "ui_options"
#define PREF_ACHIEVEMENTS "achievements"
#define PREF_FULLSCREEN "fullscreen"
#define PREF_VSYNC "vsync"
#define PREF_SHUFFLE "shuffle"
#define PREF_GAME "game"
#define PREF_MODE "mode"
#define PREF_LEVEL "level"
#define PREF_LANG "lang"

int initialize_pref(struct preferences *self, struct b6_json *json,
		    const char *path)
{
	struct b6_utf8 utf8[2];
	b6_utf8_from_ascii(&utf8[0], get_rw_dir());
	b6_utf8_from_ascii(&utf8[1], path);
	b6_initialize_utf8_string(&self->path, &b6_std_allocator);
	if (b6_extend_utf8_string(&self->path, &utf8[0]) ||
	    b6_append_utf8_string(&self->path, '/') ||
	    b6_extend_utf8_string(&self->path, &utf8[1])) {
		log_e("cannot compose path");
		b6_finalize_utf8_string(&self->path);
		return -1;
	}
	if (!(self->root = b6_json_new_object(json))) {
		log_e("cannot create json object");
		b6_finalize_utf8_string(&self->path);
		return -1;
	}
	return 0;
}

void finalize_pref(struct preferences *self)
{
	b6_finalize_utf8_string(&self->path);
	b6_json_unref_value(&self->root->up);
}

static int get_pref_bool(struct preferences *self, int default_value,
			 const struct b6_utf8 *paths, unsigned int npaths,
			 const char *id)
{
	struct b6_json_object *pref;
	struct b6_json_value *value;
	struct b6_utf8 utf8;
	if (!(pref = walk_json(self->root, paths, npaths, 0)) ||
	    !(value = b6_json_get_object(pref, b6_utf8_from_ascii(&utf8, id))))
		return default_value;
	return !b6_json_value_is_of(value, false);
}

static void set_pref_bool(struct preferences *self, int value,
			  const struct b6_utf8 *paths,
			  unsigned int npaths, const char *id)
{
	struct b6_json_object *pref;
	struct b6_utf8 utf8;
	if ((pref = walk_json(self->root, paths, npaths, 1)))
		b6_json_set_object(pref, b6_utf8_from_ascii(&utf8, id), value ?
				   &pref->json->json_true.up :
				   &pref->json->json_false.up);
}

static const struct b6_utf8 *get_pref_string(struct preferences *self,
					     const struct b6_utf8 *paths,
					     unsigned int npaths,
					     const char *id)
{
	struct b6_json_object *pref;
	struct b6_json_string *str;
	struct b6_utf8 utf8;
	b6_utf8_from_ascii(&utf8, id);
	if (!(pref = walk_json(self->root, paths, npaths, 0)) ||
	    !(str = b6_json_get_object_as(pref, &utf8, string)))
		return NULL;
	return b6_json_get_string(str);
}

static void set_pref_string(struct preferences *self,
			    const struct b6_utf8 *utf8,
			    const struct b6_utf8 *paths, unsigned int npaths,
			    const char *id)
{
	struct b6_json_object *pref;
	struct b6_json_string *str;
	enum b6_json_error error;
	struct b6_utf8 id_utf8;
	if (!(pref = walk_json(self->root, paths, npaths, 1)))
		return;
	b6_utf8_from_ascii(&id_utf8, id);
	if ((str = b6_json_get_object_as(pref, &id_utf8, string)))
		b6_json_set_string(str, utf8);
	else if (!(str = b6_json_new_string(pref->json, utf8)))
		log_e("cannot allocate %s", id);
	else if ((error = b6_json_set_object(pref, &id_utf8, &str->up))) {
		log_e("cannot set %s (%s)", id, b6_json_strerror(error));
		b6_json_unref_value(&str->up);
	}
}

static const struct b6_utf8 video_path[] = {
	B6_DEFINE_UTF8(PREF_VIDEO_OPTIONS),
};

int get_pref_fullscreen(struct preferences *self)
{
	return get_pref_bool(self, 0, video_path, b6_card_of(video_path),
			     PREF_FULLSCREEN);
}

void set_pref_fullscreen(struct preferences *self, int value)
{
	set_pref_bool(self, value, video_path, b6_card_of(video_path),
		      PREF_FULLSCREEN);
}

int get_pref_vsync(struct preferences *self)
{
	return get_pref_bool(self, 1, video_path, b6_card_of(video_path),
			     PREF_VSYNC);
}

void set_pref_vsync(struct preferences *self, int value)
{
	set_pref_bool(self, value, video_path, b6_card_of(video_path),
		      PREF_VSYNC);
}

static const struct b6_utf8 game_path[] = { B6_DEFINE_UTF8(PREF_GAME_OPTIONS) };

int get_pref_shuffle(struct preferences *self)
{
	return get_pref_bool(self, 0, game_path, b6_card_of(game_path),
			     PREF_SHUFFLE);
}

void set_pref_shuffle(struct preferences *self, int value)
{
	set_pref_bool(self, value, game_path, b6_card_of(game_path),
		      PREF_SHUFFLE);
}

const struct b6_utf8 *get_pref_game(struct preferences *self)
{
	return get_pref_string(self, game_path, b6_card_of(game_path),
			       PREF_GAME);
}

void set_pref_game(struct preferences *self, const struct b6_utf8 *utf8)
{
	set_pref_string(self, utf8, game_path, b6_card_of(game_path),
			PREF_GAME);
}

const struct b6_utf8 *get_pref_mode(struct preferences *self)
{
	return get_pref_string(self, game_path, b6_card_of(game_path),
			       PREF_MODE);
}

void set_pref_mode(struct preferences *self, const struct b6_utf8 *utf8)
{
	set_pref_string(self, utf8, game_path, b6_card_of(game_path),
			PREF_MODE);
}

static const struct b6_utf8 ui_path[] = { B6_DEFINE_UTF8(PREF_UI_OPTIONS) };

const struct b6_utf8 *get_pref_lang(struct preferences *self)
{
	return get_pref_string(self, ui_path, b6_card_of(ui_path), PREF_LANG);
}

void set_pref_lang(struct preferences *self, const struct b6_utf8 *utf8)
{
	set_pref_string(self, utf8, ui_path, b6_card_of(ui_path), PREF_LANG);
}

static const struct b6_utf8 level_utf8 = B6_DEFINE_UTF8(PREF_LEVEL);

void set_pref_level(struct preferences *self, unsigned int level,
		    const struct b6_utf8 *game, const struct b6_utf8 *mode)
{
	const struct b6_utf8 path[] = {
		B6_DEFINE_UTF8(PREF_ACHIEVEMENTS),
		B6_CLONE_UTF8(game),
		B6_CLONE_UTF8(mode),
	};
	struct b6_json_object *pref;
	struct b6_json_number *number;
	enum b6_json_error error;
	if (!(pref = walk_json(self->root, path, b6_card_of(path), 1)))
		return;
	if (!(number = b6_json_new_number(self->root->json, level))) {
		log_e("cannot allocate number");
		return;
	}
	if ((error = b6_json_set_object(pref, &level_utf8, &number->up))) {
		log_e("cannot set number (%s)", b6_json_strerror(error));
		b6_json_unref_value(&number->up);
	}
}

unsigned int get_pref_level(const struct preferences *self,
			    const struct b6_utf8 *game,
			    const struct b6_utf8 *mode)
{
	const struct b6_utf8 path[] = {
		B6_DEFINE_UTF8(PREF_ACHIEVEMENTS),
		B6_CLONE_UTF8(game),
		B6_CLONE_UTF8(mode),
	};
	struct b6_json_object *pref;
	struct b6_json_number *number;
	if (!(pref = walk_json(self->root, path, b6_card_of(path), 0)) ||
	    !(number = b6_json_get_object_as(pref, &level_utf8, number)))
		return 0;
	return b6_json_get_number(number);
}

int load_pref(struct preferences *self)
{
	struct ifstream fs;
	unsigned char buf[1024];
	struct izstream zs;
	struct json_istream js;
	struct b6_json_parser_info info;
	enum b6_json_error error;
	if (initialize_ifstream(&fs, (char*)self->path.utf8.ptr)) {
		log_e("cannot create file stream");
		return -1;
	}
	if (initialize_izstream(&zs, &fs.istream, buf, sizeof(buf))) {
		log_e("cannot create decompression stream");
		finalize_ifstream(&fs);
		return -1;
	}
	setup_json_istream(&js, &zs.up);
	if ((error = b6_json_parse_object(self->root, &js.up, &info)))
		log_e("%s", b6_json_strerror(error));
	finalize_izstream(&zs);
	finalize_ifstream(&fs);
	return -(error != B6_JSON_OK);
}

int save_pref(struct preferences *self)
{
	struct ofstream fs;
	unsigned char buf[4096];
	struct ozstream zs;
	struct json_ostream js;
	struct b6_json_default_serializer serializer;
	enum b6_json_error error;
	initialize_ofstream(&fs, self->path.utf8.ptr);
	if (initialize_ozstream(&zs, &fs.ostream, buf, sizeof(buf))) {
		finalize_ofstream(&fs);
		log_e("cannot create compression stream");
		return -1;
	}
	initialize_json_ostream(&js, &zs.up);
	b6_json_setup_default_serializer(&serializer);
	error = b6_json_serialize_object(self->root, &js.up, &serializer.up);
	if (error)
		log_e("%s", b6_json_strerror(error));
	finalize_json_ostream(&js);
	finalize_ozstream(&zs);
	finalize_ofstream(&fs);
	return 0;
}
