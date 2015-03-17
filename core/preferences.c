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
	char *ptr = self->path;
	unsigned int len = b6_card_of(self->path);
	const char *src = get_rw_dir();
	while (*src) {
		if (!len--)
			return -2;
		*ptr++ = *src++;
	}
	if (!len)
		return -2;
	*ptr++ = '/';
	len -= 1;
	do
		if (!len--)
			return -2;
	while ((*ptr++ = *path++));
	self->root = b6_json_new_object(json);
	return -!self->root;
}

void finalize_pref(struct preferences *self)
{
	b6_json_unref_value(&self->root->up);
}

static int get_pref_bool(struct preferences *self, int default_value,
			 const void **path_utf8, unsigned int *path_size,
			 unsigned int npaths, const char *id)
{
	struct b6_json_object *pref;
	struct b6_json_value *value;
	if (!(pref = walk_json(self->root, path_utf8, path_size, npaths, 0)) ||
	    !(value = b6_json_get_object(pref, id)))
		return default_value;
	return !b6_json_value_is_of(value, false);
}

static void set_pref_bool(struct preferences *self, int value,
			  const void **path_utf8, unsigned int *path_size,
			  unsigned int npaths, const char *id)
{
	struct b6_json_object *pref;
	if ((pref = walk_json(self->root, path_utf8, path_size, npaths, 1)))
		b6_json_set_object(pref, id, value ?
				   &pref->json->json_true.up :
				   &pref->json->json_false.up);
}

static const void *get_pref_string(struct preferences *self, unsigned int *size,
				   const void **path_utf8,
				   unsigned int *path_size,
				   unsigned int npaths,
				   const char *id)
{
	struct b6_json_object *pref;
	struct b6_json_string *str;
	if (!(pref = walk_json(self->root, path_utf8, path_size, npaths, 0)) ||
	    !(str = b6_json_get_object_as(pref, id, string)))
		return NULL;
	*size = b6_json_string_size(str);
	return b6_json_string_utf8(str);
}

static void set_pref_string(struct preferences *self,
			    const void *utf8, unsigned int size,
			    const void **path_utf8, unsigned int *path_size,
			    unsigned int npaths, const char *id)
{
	struct b6_json_object *pref;
	struct b6_json_string *str;
	enum b6_json_error error;
	if (!(pref = walk_json(self->root, path_utf8, path_size, npaths, 1)))
		return;
	if ((str = b6_json_get_object_as(pref, id, string)))
		b6_json_set_string(str, utf8, size);
	else if (!(str = b6_json_new_string(pref->json, NULL)))
		log_e("cannot allocate %s", id);
	else if (b6_json_set_string(str, utf8, size)) {
		log_e("cannot assign %s", id);
		b6_json_unref_value(&str->up);
	} else if ((error = b6_json_set_object(pref, id, &str->up))) {
		log_e("cannot set %s (%s)", id, b6_json_strerror(error));
		b6_json_unref_value(&str->up);
	}
}

static const void *video_path[] = { PREF_VIDEO_OPTIONS };
static unsigned int video_size[] = { sizeof(PREF_VIDEO_OPTIONS) - 1 };

int get_pref_fullscreen(struct preferences *self)
{
	return get_pref_bool(self, 0, video_path, video_size,
			     b6_card_of(video_path), PREF_FULLSCREEN);
}

void set_pref_fullscreen(struct preferences *self, int value)
{
	set_pref_bool(self, value, video_path, video_size,
		      b6_card_of(video_path), PREF_FULLSCREEN);
}

int get_pref_vsync(struct preferences *self)
{
	return get_pref_bool(self, 1, video_path, video_size,
			     b6_card_of(video_path), PREF_VSYNC);
}

void set_pref_vsync(struct preferences *self, int value)
{
	set_pref_bool(self, value, video_path, video_size,
		      b6_card_of(video_path), PREF_VSYNC);
}

static const void *game_path[] = { PREF_GAME_OPTIONS };
static unsigned int game_size[] = { sizeof(PREF_GAME_OPTIONS) - 1 };

int get_pref_shuffle(struct preferences *self)
{
	return get_pref_bool(self, 0, game_path, game_size,
			     b6_card_of(game_path), PREF_SHUFFLE);
}

void set_pref_shuffle(struct preferences *self, int value)
{
	set_pref_bool(self, value, game_path, game_size, b6_card_of(game_path),
		      PREF_SHUFFLE);
}

const void *get_pref_game(struct preferences *self, unsigned int *size)
{
	return get_pref_string(self, size, game_path, game_size,
			       b6_card_of(game_path), PREF_GAME);
}

void set_pref_game(struct preferences *self, const void *utf8,
		   unsigned int size)
{
	set_pref_string(self, utf8, size, game_path, game_size,
			b6_card_of(game_path), PREF_GAME);
}

const void *get_pref_mode(struct preferences *self, unsigned int *size)
{
	return get_pref_string(self, size, game_path, game_size,
			       b6_card_of(game_path), PREF_MODE);
}

void set_pref_mode(struct preferences *self, const void *utf8,
		   unsigned int size)
{
	set_pref_string(self, utf8, size, game_path, game_size,
			b6_card_of(game_path), PREF_MODE);
}

static const void *ui_path[] = { PREF_UI_OPTIONS };
static unsigned int ui_size[] = { sizeof(PREF_UI_OPTIONS) - 1 };

const void *get_pref_lang(struct preferences *self, unsigned int *size)
{
	return get_pref_string(self, size, ui_path, ui_size,
			       b6_card_of(ui_path), PREF_LANG);
}

void set_pref_lang(struct preferences *self, const void *utf8,
		   unsigned int size)
{
	set_pref_string(self, utf8, size, ui_path, ui_size, b6_card_of(ui_path),
			PREF_LANG);
}

void set_pref_level(struct preferences *self, unsigned int level,
		    const void *game_utf8, unsigned int game_size,
		    const void *mode_utf8, unsigned int mode_size)
{
	const void *path_utf8[] = { PREF_ACHIEVEMENTS, game_utf8, mode_utf8 };
	unsigned int path_size[] = {
		sizeof(PREF_ACHIEVEMENTS) - 1, game_size, mode_size
	};
	struct b6_json_object *pref;
	struct b6_json_number *number;
	enum b6_json_error error;
	if (!(pref = walk_json(self->root, path_utf8, path_size,
			       b6_card_of(path_utf8), 1)))
		return;
	if (!(number = b6_json_new_number(self->root->json, level))) {
		log_e("cannot allocate number");
		return;
	}
	if ((error = b6_json_set_object(pref, PREF_LEVEL, &number->up))) {
		log_e("cannot set number (%s)", b6_json_strerror(error));
		b6_json_unref_value(&number->up);
	}
}

unsigned int get_pref_level(const struct preferences *self,
			    const void *game_utf8, unsigned int game_size,
			    const void *mode_utf8, unsigned int mode_size)
{
	const void *path_utf8[] = { PREF_ACHIEVEMENTS, game_utf8, mode_utf8 };
	unsigned int path_size[] = {
		sizeof(PREF_ACHIEVEMENTS) - 1, game_size, mode_size
	};
	struct b6_json_object *pref;
	struct b6_json_number *number;
	if (!(pref = walk_json(self->root, path_utf8, path_size,
			       b6_card_of(path_utf8), 0)) ||
	    !(number = b6_json_get_object_as(pref, PREF_LEVEL, number)))
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
	if (initialize_ifstream(&fs, self->path)) {
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
	initialize_ofstream(&fs, self->path);
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
