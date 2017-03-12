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

#include "core/credits_phase.h"
#include "core/game_phase.h"
#include "core/hall_of_fame_phase.h"
#include "core/menu_phase.h"
#include "core/rgba.h"
#include "core/data.h"
#include "lib/embedded.h"
#include "lib/init.h"
#include "lib/io.h"
#include "lib/log.h"
#include "lib/std.h"
#include "utils.h"

struct default_image_data {
	struct data_entry data_entry;
	struct image_data image_data;
	const char *path;
	struct data_entry *entry;
	struct image_data *image;
};

static struct istream *get_default_image_data_entry(struct data_entry *up)
{
	struct default_image_data *self =
		b6_cast_of(up, struct default_image_data, data_entry);
	return &self->image_data.up;
}

static int default_image_data_entry_ctor(struct image_data *up, void *param)
{
	struct default_image_data *self =
		b6_cast_of(up, struct default_image_data, image_data);
	if (get_image_data_no_fallback("default", self->path, NULL,
				       &self->entry, &self->image))
		return -1;
	self->image_data.rgba = self->image->rgba;
	return 0;
}

static void default_image_data_entry_dtor(struct image_data *up)
{
	struct default_image_data *self =
		b6_cast_of(up, struct default_image_data, image_data);
	put_image_data(self->entry, self->image);
}

static const struct data_entry_ops default_entry_ops = {
	.get = get_default_image_data_entry,
};

static const struct image_data_ops default_image_data_ops = {
	.ctor = default_image_data_entry_ctor,
	.dtor = default_image_data_entry_dtor,
};

static unsigned int default_game_tile(const struct layout *l,
				      unsigned short int x,
				      unsigned short int y)
{
	static unsigned int map[] = {
		 4, 23,  7,  2, 31,  1,  0, 11, 15, 18,  8, 20, 16, 19, 12,  3,
		 4, 23,  7,  2, 31,  1,  0, 11, 15, 42,  8,  5, 16, 46, 12, 36,
		 4, 23,  7, 26, 31,  1,  0, 44, 15, 18,  8, 13, 16, 19, 12, 28,
		 4, 23,  7, 26, 31,  1,  0, 44, 15, 42,  8, 34, 16, 46, 12, 22,
		 4, 23,  7,  2, 31,  1, 24, 45, 15, 18,  8, 20, 16, 19, 14, 27,
		 4, 23,  7,  2, 31,  1, 24, 45, 15, 42,  8,  5, 16, 46, 14, 37,
		 4, 23,  7, 26, 31,  1, 24, 25, 15, 18,  8, 13, 16, 19, 14, 30,
		 4, 23,  7, 26, 31,  1, 24, 25, 15, 42,  8, 34, 16, 46, 14, 47,
		 4, 23,  7,  2, 31,  1,  0, 11, 15, 18,  8, 20, 40, 38,  6, 35,
		 4, 23,  7,  2, 31,  1,  0, 11, 15, 42,  8,  5, 40, 41,  6, 43,
		 4, 23,  7, 26, 31,  1,  0, 44, 15, 18,  8, 13, 40, 38,  6, 29,
		 4, 23,  7, 26, 31,  1,  0, 44, 15, 42,  8, 34, 40, 41,  6, 39,
		 4, 23,  7,  2, 31,  1, 24, 45, 15, 18,  8, 20, 40, 38, 32, 21,
		 4, 23,  7,  2, 31,  1, 24, 45, 15, 42,  8,  5, 40, 41, 32, 17,
		 4, 23,  7, 26, 31,  1, 24, 25, 15, 18,  8, 13, 40, 38, 32, 10,
		 4, 23,  7, 26, 31,  1, 24, 25, 15, 42,  8, 34, 40, 41, 32, 33
	};
	if (x > LEVEL_WIDTH || y > LEVEL_HEIGHT)
		return 33;
	if (get_layout(l, x, y) == LAYOUT_WALL) {
		unsigned int code = 0;
		if (get_layout(l, x    , y - 1) == LAYOUT_WALL) code |= 1 << 0;
		if (get_layout(l, x + 1, y    ) == LAYOUT_WALL) code |= 1 << 1;
		if (get_layout(l, x    , y + 1) == LAYOUT_WALL) code |= 1 << 2;
		if (get_layout(l, x - 1, y    ) == LAYOUT_WALL) code |= 1 << 3;
		if (get_layout(l, x - 1, y - 1) == LAYOUT_WALL) code |= 1 << 4;
		if (get_layout(l, x + 1, y - 1) == LAYOUT_WALL) code |= 1 << 5;
		if (get_layout(l, x + 1, y + 1) == LAYOUT_WALL) code |= 1 << 6;
		if (get_layout(l, x - 1, y + 1) == LAYOUT_WALL) code |= 1 << 7;
		return map[code];
	}
	return 9;
}

static struct rgba layout_rgba;

static int default_layout_ctor(struct image_data *up, void *layout)
{
	static const unsigned short int w = 16, h = 16;
	unsigned short int x, y;
	struct data_entry *data_entry;
	struct image_data *image_data;
	if (get_image_data("default", "private.default_game.tga", NULL,
			   &data_entry, &image_data))
		return -1;
	for (y = 0; y < LEVEL_HEIGHT; ++y) for (x = 0; x < LEVEL_WIDTH; ++x) {
		unsigned int n = default_game_tile(layout, x, y);
		copy_rgba(image_data->rgba, 544 + (n / 8) * w, 96 + (n % 8) * h,
			  w, h, &layout_rgba, x * w, y * h);
	}
	put_image_data(data_entry, image_data);
	return 0;
}

static const struct image_data_ops default_layout_image_data_ops = {
	.ctor = default_layout_ctor,
};

#define register_default_image_data(image_id, data_path, x, y, w, h, n, p) \
do { \
	static struct default_image_data data; \
	static const struct b6_utf8 data_id = \
		B6_DEFINE_UTF8(IMAGE_DATA_ID("default", image_id)); \
	data.path = "private." data_path; \
	setup_image_data(&data.image_data, &default_image_data_ops, NULL, \
			 x, y, w, h, n, p); \
	if ((register_data(&data.data_entry, &default_entry_ops, &data_id))) \
		log_e(_s("cannot register default image "), _s(image_id)); \
} while (0)

#define register_default_single_image_data(image_id, data_path, x, y, w, h) \
do { \
	static unsigned short int data_x = x; \
	static unsigned short int data_y = y; \
	register_default_image_data(image_id, data_path, &data_x, &data_y, \
				    w, h, 1, 1); \
} while (0)

#define register_default_pacman(dir, x, y) \
do { \
	register_default_image_data(GAME_PACMAN_DATA_ID("normal", dir), \
				    "default_game.tga", x, y, 32, 32, \
				    b6_card_of(y), 60000); \
	register_default_image_data(GAME_PACMAN_DATA_ID("shield", dir), \
				    "default_game.tga", x + 8, y, 32, 32, \
				    b6_card_of(y), 60000); \
} while(0)

#define register_default_ghost(n, mode, x, y) \
do { \
	register_default_single_image_data(GAME_GHOST_DATA_ID(n, mode, "e"), \
					   "default_game.tga", x, y, 32, 32); \
	register_default_single_image_data(GAME_GHOST_DATA_ID(n, mode, "w"), \
					   "default_game.tga", x, y, 32, 32); \
	register_default_single_image_data(GAME_GHOST_DATA_ID(n, mode, "s"), \
					   "default_game.tga", x, y, 32, 32); \
	register_default_single_image_data(GAME_GHOST_DATA_ID(n, mode, "n"), \
					   "default_game.tga", x, y, 32, 32); \
} while(0)

#define register_default_bonus(data_id, x, y) \
do { \
	static unsigned short int data_x[] = { x, 512 }; \
	static unsigned short int data_y[] = { y, 128 }; \
	register_default_image_data(GAME_BONUS_ ## data_id ## _DATA_ID, \
				    "default_game.tga", data_x, data_y, \
				    32, 32, 2, 500000); \
} while (0)

#define register_embedded_data(path, name) \
do { \
	static const struct b6_utf8 utf8[] = { \
		B6_DEFINE_UTF8(path), \
		B6_DEFINE_UTF8(name), \
	}; \
	static struct buffered_data_entry data; \
	struct embedded *embedded; \
	if ((embedded = lookup_embedded(&utf8[0]))) \
		register_buffered_data( \
			&data, embedded->buf, embedded->len, &utf8[1]); \
	else \
		log_e(_s("embedded path not found: "), _s(path)); \
} while (0)

#define register_embedded_level(num) \
	register_embedded_data("default_level_" num ".lev", \
			       LEVEL_DATA_ID("default", num))

#define register_cached_image(path, name) \
do { \
	static const struct b6_utf8 utf8 = B6_DEFINE_UTF8(name); \
	static struct cached_image_data data; \
	struct embedded *embedded; \
	if ((embedded = lookup_embedded(path))) { \
		register_cached_image_data( \
			&data, embedded->buf, embedded->len, &utf8); \
	} else \
		log_p(_s("embedded image path not found: "), _t(path)); \
} while (0)

#define register_external_data(_path, _name) \
do { \
	static const struct b6_utf8 utf8 = B6_DEFINE_UTF8(_name); \
	static struct buffered_data_entry data; \
	static struct membuf membuf; \
	initialize_membuf(&membuf, &b6_std_allocator); \
	if (!load_external_data(&membuf, _path)) \
		register_buffered_data(&data, membuf.buf, membuf.len, &utf8); \
} while (0)

#define register_external_cached_image(_path, _name) \
do { \
	static const struct b6_utf8 utf8 = B6_DEFINE_UTF8(_name); \
	static struct cached_image_data data; \
	static struct membuf membuf; \
	initialize_membuf(&membuf, &b6_std_allocator); \
	if (!load_external_data(&membuf, _path)) \
		register_cached_image_data( \
			&data, membuf.buf, membuf.len, &utf8); \
} while (0)

static struct data_layout_provider default_layout_provider;

struct layout_provider *get_default_layout_provider(void)
{
	return &default_layout_provider.up;
}

static int default_skin_ctor(void)
{
	static unsigned short int origin[] = { 0 };
	static unsigned short int font_x[128], font_y[128];
	static unsigned short int font_w = 12, font_h = 16;
	static struct default_image_data layout_data;
	static unsigned short int pacman_x[] = {
		  0,  32,  64,  96, 128, 160, 192, 224,
		256, 288, 320, 352, 384, 416, 448, 480
	};
	static unsigned short int pacman_y[][8] = {
		{ 128, 128, 128, 128, 128, 128, 128, 128 },
		{ 160, 160, 160, 160, 160, 160, 160, 160 },
		{ 192, 192, 192, 192, 192, 192, 192, 192 },
		{ 224, 224, 224, 224, 224, 224, 224, 224 }
	};
	static unsigned short int jewels_x[] = {
		224, 256, 288, 320, 352, 384, 416
	};
	static unsigned short int jewels_y[] = {
		256, 256, 256, 256, 256, 256, 256, 256
	};
	static unsigned short int booster_x[] = { 0, 264 };
	static unsigned short int booster_y[] = { 80, 80 };
	static unsigned short int casino_x[48], casino_y[48];
	int i, j;

	register_cached_image(B6_UTF8("default_font.tga"), IMAGE_DATA_ID(
			"default", "private.default_font.tga"));
	register_cached_image(B6_UTF8("default_game.tga"), IMAGE_DATA_ID(
			"default", "private.default_game.tga"));
	register_external_data("default/credits_music.xm.z",
			       AUDIO_DATA_ID("default", CREDITS_MUSIC_DATA_ID));

	initialize_rgba(&layout_rgba, 16 * LEVEL_WIDTH, 16 * LEVEL_HEIGHT);
	setup_image_data(&layout_data.image_data,
			 &default_layout_image_data_ops, &layout_rgba,
			 origin, origin, layout_rgba.w, layout_rgba.h, 1, 1);
	if ((register_data(&layout_data.data_entry, &default_entry_ops,
			   B6_UTF8(IMAGE_DATA_ID("default",
						       GAME_LAYOUT_DATA_ID)))))
		log_e(_s("cannot register image data " GAME_LAYOUT_DATA_ID));

	register_default_single_image_data(GAME_PANEL_DATA_ID,
					   "default_game.tga", 0, 0, 640, 80);
	register_default_single_image_data(GAME_PACGUM_DATA_ID,
					   "default_game.tga", 524, 204, 8, 8);
	register_default_single_image_data(GAME_SUPER_PACGUM_DATA_ID,
					   "default_game.tga",
					   520, 168, 16, 16);
	register_external_cached_image(
		"default/credits_background.tga.z",
		IMAGE_DATA_ID("default", CREDITS_BACKGROUND_DATA_ID));
	register_external_cached_image(
		"default/credits_sprite.tga.z",
		IMAGE_DATA_ID("default", CREDITS_PACMAN_DATA_ID));
	register_default_image_data(CREDITS_FONT_DATA_ID, "default_font.tga",
				    font_x, font_y, font_w, font_h, 1, 1);
	register_default_image_data(MENU_NORMAL_FONT_DATA_ID,
				    "default_font.tga",
				    font_x, font_y, font_w, font_h, 1, 1);
	register_default_image_data(HOF_FONT_DATA_ID, "default_font.tga",
				    font_x, font_y, font_w, font_h, 1, 1);
	register_default_image_data(GAME_FONT_DATA_ID, "default_font.tga",
				    font_x, font_y, font_w, font_h, 1, 1);
	register_default_pacman("e", pacman_x, pacman_y[0]);
	register_default_pacman("w", pacman_x, pacman_y[1]);
	register_default_pacman("s", pacman_x, pacman_y[2]);
	register_default_pacman("n", pacman_x, pacman_y[3]);
	register_default_ghost(0, "normal",   0, 256);
	register_default_ghost(0, "afraid", 128, 256);
	register_default_ghost(0, "locked", 160, 256);
	register_default_ghost(0, "zombie", 192, 256);
	register_default_ghost(1, "normal",  32, 256);
	register_default_ghost(1, "afraid", 128, 256);
	register_default_ghost(1, "locked", 160, 256);
	register_default_ghost(1, "zombie", 192, 256);
	register_default_ghost(2, "normal",  64, 256);
	register_default_ghost(2, "afraid", 128, 256);
	register_default_ghost(2, "locked", 160, 256);
	register_default_ghost(2, "zombie", 192, 256);
	register_default_ghost(3, "normal",  96, 256);
	register_default_ghost(3, "afraid", 128, 256);
	register_default_ghost(3, "locked", 160, 256);
	register_default_ghost(3, "zombie", 192, 256);

	register_default_single_image_data(
		GAME_LIFE_ICON_DATA_ID, "default_game.tga", 456, 264, 16, 16);
	register_default_single_image_data(
		GAME_SHIELD_ICON_DATA_ID, "default_game.tga", 488, 264, 16, 16);

	register_default_single_image_data(
		GAME_JEWEL_DATA_ID(1), "default_game.tga", 224, 256, 32, 32);
	register_default_single_image_data(
		GAME_JEWEL_DATA_ID(2), "default_game.tga", 256, 256, 32, 32);
	register_default_single_image_data(
		GAME_JEWEL_DATA_ID(3), "default_game.tga", 288, 256, 32, 32);
	register_default_single_image_data(
		GAME_JEWEL_DATA_ID(4), "default_game.tga", 320, 256, 32, 32);
	register_default_single_image_data(
		GAME_JEWEL_DATA_ID(5), "default_game.tga", 352, 256, 32, 32);
	register_default_single_image_data(
		GAME_JEWEL_DATA_ID(6), "default_game.tga", 384, 256, 32, 32);
	register_default_single_image_data(
		GAME_JEWEL_DATA_ID(7), "default_game.tga", 416, 256, 32, 32);

	register_default_image_data(
		GAME_BONUS_JEWELS_DATA_ID, "default_game.tga",
		jewels_x, jewels_y, 32, 32, 7, 150000);
	register_default_bonus(JEWEL1,         224, 256);
	register_default_bonus(JEWEL2,         256, 256);
	register_default_bonus(JEWEL3,         288, 256);
	register_default_bonus(JEWEL4,         320, 256);
	register_default_bonus(JEWEL5,         352, 256);
	register_default_bonus(JEWEL6,         384, 256);
	register_default_bonus(JEWEL7,         416, 256);
	register_default_bonus(LIFE,           448, 256);
	register_default_bonus(SHIELD,         480, 256);
	register_default_bonus(FORWARD,          0, 288);
	register_default_bonus(REWIND,          32, 288);
	register_default_bonus(SINGLE_BOOSTER,  64, 288);
	register_default_bonus(DOUBLE_BOOSTER,  96, 288);
	register_default_bonus(PACMAN_FAST,    128, 288);
	register_default_bonus(PACMAN_SLOW,    160, 288);
	register_default_bonus(GHOSTS_FAST,    192, 288);
	register_default_bonus(GHOSTS_SLOW,    224, 288);
	register_default_bonus(X2,             256, 288);
	register_default_bonus(CASINO,         288, 288);
	register_default_bonus(LOCKDOWN,       320, 288);
	register_default_bonus(GHOSTS_WIPEOUT, 352, 288);
	register_default_bonus(GHOSTS_BANQUET, 384, 288);
	register_default_bonus(SURPRISE,       416, 288);
	register_default_bonus(DIET,           448, 288);
	register_default_bonus(DEATH,          480, 288);
	register_default_bonus(SUPER_PACGUM,   512, 160);

	register_default_image_data(GAME_BOOSTER_DATA_ID, "default_game.tga",
				    booster_x, booster_y, 256, 8, 2, 0);

	register_default_image_data(GAME_CASINO_DATA_ID, "default_game.tga",
				    casino_x, casino_y, 22, 18,
				    b6_card_of(casino_x), 0);
	for (i = 0; i < b6_card_of(casino_x); i += 1) {
		casino_x[i] = (i % 24) * 22;
		casino_y[i] = 128 - 36 + (i / 24) * 18;
	}

	for (i = 0; i < b6_card_of(font_x); i += 1) {
		if (i < ' ' || i > 127) {
			font_x[i] = 0;
			font_y[i] = 0;
			continue;
		}
		j = i - 32;
		font_x[i] = (j % 32) * font_w;
		font_y[i] = (j / 32) * font_h;
	}

	register_embedded_level("01");
	register_embedded_level("02");
	register_embedded_level("03");
	register_embedded_level("04");
	register_embedded_level("05");
	if (reset_data_layout_provider(&default_layout_provider, "default"))
		log_p(_s("could not initialize default levels"));
	if (register_layout_provider(&default_layout_provider.up,
				     B6_UTF8("Open Greedy")))
		log_p(_s("could not register levels"));

	return 0;
}
register_init(default_skin_ctor);

static void default_skin_dtor(void)
{
	finalize_rgba(&layout_rgba);
}
register_exit(default_skin_dtor);
