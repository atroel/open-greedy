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

#include "core/rgba.h"
#include "core/data.h"
#include "core/game_phase.h"
#include "core/hall_of_fame_phase.h"
#include "core/menu_phase.h"
#include "core/hall_of_fame_phase.h"
#include "lib/init.h"
#include "lib/io.h"
#include "lib/log.h"
#include "lib/std.h"
#include "utils.h"

struct greedy_data_entry {
	struct buffered_data_entry up;
	struct membuf membuf;
};

struct greedy_cached_image_data {
	struct cached_image_data up;
	struct membuf membuf;
};

struct greedy_image_data {
	struct data_entry up;
	struct image_data image_data;
	const char *data_id;
	struct data_entry *entry;
	struct image_data *image;
	unsigned int count;
};

static struct istream *get_greedy_image_data_entry(struct data_entry *up)
{
	return &b6_cast_of(up, struct greedy_image_data, up)->image_data.up;
}

static int greedy_image_data_entry_ctor(struct image_data *up, void *param)
{
	struct greedy_image_data *self =
		b6_cast_of(up, struct greedy_image_data, image_data);
	if (get_image_data_no_fallback("greedy", self->data_id, NULL,
				       &self->entry, &self->image))
		return -1;
	self->image_data.rgba = self->image->rgba;
	return 0;
}

static void greedy_image_data_entry_dtor(struct image_data *up)
{
	struct greedy_image_data *self =
		b6_cast_of(up, struct greedy_image_data, image_data);
	put_image_data(self->entry, self->image);
}

static const struct image_data_ops greedy_image_ops = {
	.ctor = greedy_image_data_entry_ctor,
	.dtor = greedy_image_data_entry_dtor,
};

static unsigned int greedy_game_tile(const struct layout *l,
				     unsigned short int x, unsigned short int y)
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

static int greedy_layout_ctor(struct image_data *up, void *layout)
{
	static const unsigned short int w = 16, h = 16;
	struct greedy_image_data *self =
		b6_cast_of(up, struct greedy_image_data, image_data);
	struct rgba *layout_rgba = (struct rgba*)up->rgba;
	struct data_entry *data_entry;
	struct image_data *image_data;
	unsigned short int x, y;
	if (self->count)
		goto done;
	if (initialize_rgba(layout_rgba, w * LEVEL_WIDTH, h * LEVEL_HEIGHT)) {
		self->count = 0;
		return -1;
	}
	if (get_image_data_no_fallback("greedy", self->data_id, NULL,
				       &data_entry, &image_data)) {
		finalize_rgba(layout_rgba);
		return -1;
	}
	for (y = 0; y < LEVEL_HEIGHT; ++y) for (x = 0; x < LEVEL_WIDTH; ++x) {
		unsigned n = greedy_game_tile(layout, x, y);
		copy_rgba(image_data->rgba, (n / 8) * w, (n % 8) * h, w, h,
			  layout_rgba, x * w, y * h);
	}
	put_image_data(data_entry, image_data);
done:
	self->count += 1;
	return 0;
}

static void greedy_layout_dtor(struct image_data *up)
{
	if (!--b6_cast_of(up, struct greedy_image_data, image_data)->count)
		finalize_rgba((struct rgba*)up->rgba);
}

static const struct image_data_ops greedy_layout_ops = {
	.ctor = greedy_layout_ctor,
	.dtor = greedy_layout_dtor,
};

static int register_greedy_image_data(struct greedy_image_data *self,
				      const char *name,
				      const char *data_id,
				      const struct image_data_ops *ops,
				      unsigned short int *x,
				      unsigned short int *y,
				      unsigned short int w,
				      unsigned short int h,
				      unsigned int length,
				      unsigned long long int period)
{
	static const struct data_entry_ops greedy_image_data_entry_ops = {
		.get = get_greedy_image_data_entry,
	};
	int retval;
	self->image_data.up.ops = NULL;
	self->image_data.ops = ops;
	self->image_data.rgba = NULL;
	self->image_data.x = x;
	self->image_data.y = y;
	self->image_data.w = w;
	self->image_data.h = h;
	self->image_data.length = length;
	self->image_data.period = period;
	self->data_id = data_id;
	self->count = 0;
	retval = register_data(&self->up, &greedy_image_data_entry_ops, name);
	if (retval)
		log_e("could not register %s", name);
	return retval;
}

static void setup_greedy_image_xy(struct greedy_image_data *self,
				  unsigned short int x, unsigned short int y,
				  unsigned short int dx, unsigned short int dy)
{
	unsigned short int *px = self->image_data.x;
	unsigned short int *py = self->image_data.y;
	unsigned int length = self->image_data.length;
	dx *= self->image_data.w;
	dy *= self->image_data.h;
	while (length--) { *px++ = x; x += dx; *py++ = y; y += dy; }
}

static void register_greedy_data_entry(struct greedy_data_entry *self,
				       const char *name, const char *path)
{
	initialize_membuf(&self->membuf, &b6_std_allocator);
	if (!load_external_data(&self->membuf, path))
		register_buffered_data(&self->up, self->membuf.buf,
				       self->membuf.len, name);
}

#define GREEDY_FONT "private.font"
#define GREEDY_GAME_PANEL "private.game.panel"
#define GREEDY_GAME_PATTERN "private.game.pattern"
#define GREEDY_GAME_SPRITE "private.game.sprite"
#define GREEDY_GAME_ANIMATION "private.game.animation"
#define GREEDY_GAME_OPTION "private.game.option"
#define GREEDY_MENU_BACKGROUND "private.menu.background"
#define GREEDY_MENU_TITLE "private.menu.title"
#define GREEDY_MENU_SPRITE "private.menu.sprite"
#define GREEDY_HOF_BACKGROUND "private.hof.background"
#define GREEDY_HOF_PANEL "private.hof.panel"

#define greedy_image_resource(_name, _path) \
do { \
	static struct greedy_cached_image_data data; \
	initialize_membuf(&data.membuf, &b6_std_allocator); \
	if (!load_external_data(&data.membuf, "greedy/image/" _path ".tga.z")) \
		register_cached_image_data( \
			&data.up, data.membuf.buf, data.membuf.len, \
			IMAGE_DATA_ID("greedy", _name)); \
} while (0)

#define register_greedy_audio(name, path) do { \
	static struct greedy_data_entry data; \
	register_greedy_data_entry(&data, AUDIO_DATA_ID("greedy", name), \
				   "greedy/audio/" path ".z"); \
} while(0)

#define register_greedy_level(num) \
	do { \
		static struct greedy_data_entry data; \
		register_greedy_data_entry( \
			&data, LEVEL_DATA_ID("greedy", num), \
			"greedy/level/level_" num ".lev.z"); \
	} while(0)

#define register_greedy_pacman(m, d, x, y) do { \
	static struct greedy_image_data data; \
	static unsigned short int data_x[10], data_y[10]; \
	register_greedy_image_data( \
		&data, IMAGE_DATA_ID("greedy", GAME_PACMAN_DATA_ID(m, d)), \
		GREEDY_GAME_SPRITE, &greedy_image_ops, data_x, data_y, \
		32, 32, 10, 50000); \
	setup_greedy_image_xy(&data, x, y, 1, 0); \
} while (0)

#define GREEDY_GHOST_MODE_DATA_ID(n, mode) { \
	IMAGE_DATA_ID("greedy", GAME_GHOST_DATA_ID(n, mode, "e")), \
	IMAGE_DATA_ID("greedy", GAME_GHOST_DATA_ID(n, mode, "w")), \
	IMAGE_DATA_ID("greedy", GAME_GHOST_DATA_ID(n, mode, "s")), \
	IMAGE_DATA_ID("greedy", GAME_GHOST_DATA_ID(n, mode, "n")), \
}

#define GREEDY_GHOST_DATA_ID(n) { \
	GREEDY_GHOST_MODE_DATA_ID(n, "normal"), \
	GREEDY_GHOST_MODE_DATA_ID(n, "afraid"), \
	GREEDY_GHOST_MODE_DATA_ID(n, "locked"), \
	GREEDY_GHOST_MODE_DATA_ID(n, "zombie"), \
}

#define register_greedy_single(data_id, entry, x, y, w, h) do { \
	static struct greedy_image_data data; \
	static unsigned short int data_x = x, data_y = y; \
	register_greedy_image_data( \
		&data, IMAGE_DATA_ID("greedy", data_id), entry, \
		&greedy_image_ops, &data_x, &data_y, w, h, 1, 0); \
} while (0)

#define register_greedy_jewel(n) register_greedy_single( \
	GAME_JEWEL_DATA_ID(n), GREEDY_GAME_OPTION, n * 64 - 32, 32, 32, 32)

#define register_greedy_points(n) register_greedy_single( \
	GAME_POINTS_DATA_ID(n ## 00), GREEDY_GAME_SPRITE, \
	320, 224 + n * 9 - 9, 27, 9)

#define register_greedy_info(data_id, x, y) register_greedy_single( \
	data_id, GREEDY_GAME_OPTION, x, y, 32, 32)

#define register_greedy_bonus(data_id, x, y, n, p) do { \
	static struct greedy_image_data data; \
	static unsigned short int data_x[n], data_y[n]; \
	register_greedy_image_data( \
		&data, \
		IMAGE_DATA_ID("greedy", GAME_BONUS_ ## data_id ## _DATA_ID), \
		GREEDY_GAME_OPTION, &greedy_image_ops, data_x, data_y, \
		32, 32, n, p); \
	setup_greedy_image_xy(&data, x, y, 1, 0); \
} while (0)

static void greedy_skin_dtor(void)
{
}
register_exit(greedy_skin_dtor);

static int greedy_skin_ctor(void)
{
	int i, j, k;

	register_greedy_level("01"); register_greedy_level("02");
	register_greedy_level("03"); register_greedy_level("04");
	register_greedy_level("05"); register_greedy_level("06");
	register_greedy_level("07"); register_greedy_level("08");
	register_greedy_level("09"); register_greedy_level("10");
	register_greedy_level("11"); register_greedy_level("12");
	register_greedy_level("13"); register_greedy_level("14");
	register_greedy_level("15"); register_greedy_level("16");
	register_greedy_level("17"); register_greedy_level("18");
	register_greedy_level("19"); register_greedy_level("20");
	register_greedy_level("21"); register_greedy_level("22");
	register_greedy_level("23"); register_greedy_level("24");
	register_greedy_level("25"); register_greedy_level("26");
	register_greedy_level("27"); register_greedy_level("28");
	register_greedy_level("29"); register_greedy_level("30");
	register_greedy_level("31"); register_greedy_level("32");
	register_greedy_level("33"); register_greedy_level("34");
	register_greedy_level("35"); register_greedy_level("36");
	register_greedy_level("37"); register_greedy_level("38");
	register_greedy_level("39"); register_greedy_level("40");
	register_greedy_level("41"); register_greedy_level("42");
	register_greedy_level("43"); register_greedy_level("44");
	register_greedy_level("45"); register_greedy_level("46");
	register_greedy_level("47"); register_greedy_level("48");
	register_greedy_level("49"); register_greedy_level("50");
	register_greedy_level("51"); register_greedy_level("52");
	register_greedy_level("53"); register_greedy_level("54");
	register_greedy_level("55"); register_greedy_level("56");
	register_greedy_level("57"); register_greedy_level("58");
	register_greedy_level("59"); register_greedy_level("60");
	register_greedy_level("61"); register_greedy_level("62");
	register_greedy_level("63"); register_greedy_level("64");
	register_greedy_level("65"); register_greedy_level("66");
	register_greedy_level("67"); register_greedy_level("68");
	register_greedy_level("69"); register_greedy_level("70");
	register_greedy_level("71"); register_greedy_level("72");
	register_greedy_level("73"); register_greedy_level("74");
	register_greedy_level("75"); register_greedy_level("76");
	register_greedy_level("77"); register_greedy_level("78");

	greedy_image_resource(GREEDY_FONT, "font");
	greedy_image_resource(GREEDY_GAME_PANEL, "game_panel");
	greedy_image_resource(GREEDY_GAME_PATTERN, "game_pattern");
	greedy_image_resource(GREEDY_GAME_SPRITE, "game_sprite");
	greedy_image_resource(GREEDY_GAME_ANIMATION, "game_animation");
	greedy_image_resource(GREEDY_GAME_OPTION, "game_option");
	greedy_image_resource(GREEDY_MENU_BACKGROUND, "menu_background");
	greedy_image_resource(GREEDY_MENU_TITLE, "menu_title");
	greedy_image_resource(GREEDY_MENU_SPRITE, "menu_sprite");
	greedy_image_resource(GREEDY_HOF_BACKGROUND, "hiscores_background");
	greedy_image_resource(GREEDY_HOF_PANEL, "hiscores_panel");

	static unsigned short int font_x[3][128];
	static unsigned short int font_y[3][128];
	static const unsigned short font_w = 16, font_h = 16;
	for (j = 0; j < b6_card_of(font_x); j += 1)
		for (i = 0; i < b6_card_of(font_x[j]); i += 1) {
			if (i < ' ' || i > ']') {
				font_x[j][i] = 0;
				font_y[j][i] = 0;
				continue;
			}
			if (i == '\"')
				k = '\'' - 32;
			else if (i == '\'')
				k = '\"' - 32;
			else
				k = i - 32;
			font_x[j][i] = (k % 31) * font_w;
			font_y[j][i] = (j * 2 + (k / 31)) * font_h;
		}

	static struct greedy_image_data menu_normal_font_data;
	register_greedy_image_data(
		&menu_normal_font_data,
		IMAGE_DATA_ID("greedy", MENU_NORMAL_FONT_DATA_ID),
		GREEDY_FONT, &greedy_image_ops, font_x[0], font_y[0],
		font_w, font_h, b6_card_of(font_x[1]), 0);

	static struct greedy_image_data menu_bright_font_data;
	register_greedy_image_data(
		&menu_bright_font_data,
		IMAGE_DATA_ID("greedy", MENU_BRIGHT_FONT_DATA_ID),
		GREEDY_FONT, &greedy_image_ops, font_x[2], font_y[2],
		font_w, font_h, b6_card_of(font_x[2]), 0);

	register_greedy_single(MENU_BACKGROUND_DATA_ID, GREEDY_MENU_BACKGROUND,
			       0, 0, 640, 480);
	register_greedy_single(MENU_TITLE_DATA_ID, GREEDY_MENU_TITLE,
			       0, 0, 500, 150);
	register_greedy_single(MENU_PACMAN_DATA_ID, GREEDY_MENU_SPRITE,
			       3, 3, 96, 96);
	register_greedy_single(MENU_GHOST_DATA_ID, GREEDY_MENU_SPRITE,
			       0, 100, 100, 124);
	register_greedy_audio(HOF_MUSIC_DATA_ID, "music_score.mod");
	register_greedy_audio(MENU_MUSIC_DATA_ID, "music_menu.mod");

	static struct greedy_image_data hof_font_data;
	register_greedy_image_data(
		&hof_font_data, IMAGE_DATA_ID("greedy", HOF_FONT_DATA_ID),
		GREEDY_FONT, &greedy_image_ops, font_x[0], font_y[0],
		font_w, font_h, b6_card_of(font_x[0]), 0);

	register_greedy_single(HOF_BACKGROUND_DATA_ID, GREEDY_HOF_BACKGROUND,
			       0, 0, 640, 480);
	register_greedy_single(HOF_PANEL_DATA_ID, GREEDY_HOF_PANEL,
			       0, 0, 112, 480);

	static struct greedy_image_data game_font_data;
	register_greedy_image_data(
		&game_font_data, IMAGE_DATA_ID("greedy", GAME_FONT_DATA_ID),
		GREEDY_FONT, &greedy_image_ops, font_x[1], font_y[1],
		font_w, font_h, b6_card_of(font_x[1]), 0);

	register_greedy_audio(GAME_MUSIC_DATA_ID, "music_game.mod");
	register_greedy_audio(GAME_SOUND_GHOST_AFRAID_DATA_ID,
			      "sound_afraid.wav");
	register_greedy_audio(GAME_SOUND_BONUS_DATA_ID, "sound_bonus.wav");
	register_greedy_audio(GAME_SOUND_BOOSTER_DRAIN_DATA_ID,
			      "sound_booster.wav");
	register_greedy_audio(GAME_SOUND_GHOST_DEATH_DATA_ID,
			      "sound_dead ghost.wav");
	register_greedy_audio(GAME_SOUND_PACMAN_DEATH_DATA_ID,
			      "sound_dead hero.wav");
	register_greedy_audio(GAME_SOUND_BOOSTER_FULL_DATA_ID,
			      "sound_honk.wav");
	register_greedy_audio(GAME_SOUND_SHIELD_DATA_ID,
			      "sound_invincible.wav");
	register_greedy_audio(GAME_SOUND_JOKER_DATA_ID, "sound_joker.wav");
	register_greedy_audio(GAME_SOUND_LIFE_DATA_ID, "sound_life.wav");
	register_greedy_audio(GAME_SOUND_BOOSTER_RELOAD_DATA_ID,
			      "sound_reload.wav");
	register_greedy_audio(GAME_SOUND_RETURN_DATA_ID, "sound_return.wav");
	register_greedy_audio(GAME_SOUND_RING_DATA_ID, "sound_ring.wav");
	register_greedy_audio(GAME_SOUND_CASINO_DATA_ID, "sound_roll.wav");
	register_greedy_audio(GAME_SOUND_TELEPORT_DATA_ID, "sound_telepod.wav");
	register_greedy_audio(GAME_SOUND_WIPEOUT_DATA_ID,
			      "sound_tranquility.wav");

	register_greedy_pacman("normal", "e", 0, 0);
	register_greedy_pacman("normal", "w", 0, 32);
	register_greedy_pacman("normal", "s", 0, 64);
	register_greedy_pacman("normal", "n", 0, 96);
	register_greedy_pacman("shield", "e", 0, 128);
	register_greedy_pacman("shield", "w", 0, 160);
	register_greedy_pacman("shield", "s", 0, 192);
	register_greedy_pacman("shield", "n", 0, 224);

	static const char *ghost_data_id[4][4][4] = {
		GREEDY_GHOST_DATA_ID(0),
		GREEDY_GHOST_DATA_ID(1),
		GREEDY_GHOST_DATA_ID(2),
		GREEDY_GHOST_DATA_ID(3),
	};
	static struct greedy_image_data ghost_data[4][4][4];
	static unsigned short int ghost_normal_x[4][6];
	static unsigned short int ghost_normal_y[4][6];
	static unsigned short int ghost_afraid_x[6];
	static unsigned short int ghost_afraid_y[6];
	static unsigned short int ghost_locked_x[6];
	static unsigned short int ghost_locked_y[6];
	static unsigned short int ghost_zombie_x;
	static unsigned short int ghost_zombie_y;
	for (i = 0; i < b6_card_of(ghost_data); i += 1)
		for (j = 0; j < b6_card_of(ghost_data[i][0]); j += 1) {
			register_greedy_image_data(
				&ghost_data[i][0][j], ghost_data_id[i][0][j],
				GREEDY_GAME_SPRITE, &greedy_image_ops,
				ghost_normal_x[i], ghost_normal_y[i], 32, 32,
				b6_card_of(ghost_normal_x[i]), 75000);
			register_greedy_image_data(
				&ghost_data[i][1][j], ghost_data_id[i][1][j],
				GREEDY_GAME_SPRITE, &greedy_image_ops,
				ghost_afraid_x, ghost_afraid_y, 32, 32,
				b6_card_of(ghost_afraid_x), 75000);
			register_greedy_image_data(
				&ghost_data[i][2][j], ghost_data_id[i][2][j],
				GREEDY_GAME_SPRITE, &greedy_image_ops,
				ghost_locked_x, ghost_locked_y, 32, 32,
				b6_card_of(ghost_locked_x), 75000);
			register_greedy_image_data(
				&ghost_data[i][3][j], ghost_data_id[i][3][j],
				GREEDY_GAME_SPRITE, &greedy_image_ops,
				&ghost_zombie_x, &ghost_zombie_y, 32, 32,
				1, 1);
		}
	for (i = 0; i < b6_card_of(ghost_normal_x); i += 1)
		setup_greedy_image_xy(&ghost_data[i][0][0], 320, i * 32, 1, 0);
	setup_greedy_image_xy(&ghost_data[0][1][0], 320, 4 * 32, 1, 0);
	setup_greedy_image_xy(&ghost_data[0][2][0], 320, 5 * 32, 1, 0);
	setup_greedy_image_xy(&ghost_data[0][3][0], 480, 296, 0, 0);

	static struct greedy_image_data booster_data;
	static unsigned short int booster_x[2], booster_y[2];
	register_greedy_image_data(
		&booster_data, IMAGE_DATA_ID("greedy", GAME_BOOSTER_DATA_ID),
		GREEDY_GAME_SPRITE, &greedy_image_ops, booster_x, booster_y,
		185, 10, 2, 0);
	setup_greedy_image_xy(&booster_data, 320, 192, 0, 1);

	static struct greedy_image_data casino_data;
	static unsigned short int casino_x[56], casino_y[b6_card_of(casino_x)];
	register_greedy_image_data(
		&casino_data, IMAGE_DATA_ID("greedy", GAME_CASINO_DATA_ID),
		GREEDY_GAME_SPRITE, &greedy_image_ops, casino_x, casino_y,
		22, 18, b6_card_of(casino_x), 0);
	for (i = 0; i < b6_card_of(casino_x); i += 1) {
		int n = (i + 12) % b6_card_of(casino_x);
		casino_x[i] = (n % 14) * casino_data.image_data.w;
		casino_y[i] = 256 + (n / 14) * casino_data.image_data.h;
	}

	register_greedy_points(1);
	register_greedy_points(2);
	register_greedy_points(3);
	register_greedy_points(4);
	register_greedy_points(5);
	register_greedy_points(6);
	register_greedy_points(7);
	register_greedy_points(8);
	register_greedy_points(9);

	register_greedy_info(GAME_INFO_JEWEL1_DATA_ID,           0, 64);
	register_greedy_info(GAME_INFO_JEWEL2_DATA_ID,          32, 64);
	register_greedy_info(GAME_INFO_JEWEL3_DATA_ID,          64, 64);
	register_greedy_info(GAME_INFO_JEWEL4_DATA_ID,          96, 64);
	register_greedy_info(GAME_INFO_JEWEL5_DATA_ID,         128, 64);
	register_greedy_info(GAME_INFO_JEWEL6_DATA_ID,         160, 64);
	register_greedy_info(GAME_INFO_JEWEL7_DATA_ID,         192, 64);
	register_greedy_info(GAME_INFO_REWIND_DATA_ID,         224, 64);
	register_greedy_info(GAME_INFO_FORWARD_DATA_ID,        256, 64);
	register_greedy_info(GAME_INFO_GHOSTS_SLOW_DATA_ID,    288, 64);
	register_greedy_info(GAME_INFO_GHOSTS_FAST_DATA_ID,    320, 64);
	register_greedy_info(GAME_INFO_WIPEOUT_DATA_ID,        352, 64);
	register_greedy_info(GAME_INFO_LOCKDOWN_DATA_ID,       384, 64);
	register_greedy_info(GAME_INFO_SUPER_PACGUM_DATA_ID,   416, 64);
	register_greedy_info(GAME_INFO_BANQUET_DATA_ID,        448, 64);
	register_greedy_info(GAME_INFO_PACMAN_SLOW_DATA_ID,    480, 64);
	register_greedy_info(GAME_INFO_PACMAN_FAST_DATA_ID,    512, 64);
	register_greedy_info(GAME_INFO_SHIELD_DATA_ID,         544, 64);
	register_greedy_info(GAME_INFO_CASINO_DATA_ID,         576, 64);
	register_greedy_info(GAME_INFO_LIFE_DATA_ID,           608, 64);
	register_greedy_info(GAME_INFO_DEATH_DATA_ID,          640, 64);
	register_greedy_info(GAME_INFO_DIET_DATA_ID,           672, 64);
	register_greedy_info(GAME_INFO_SINGLE_BOOSTER_DATA_ID, 704, 64);
	register_greedy_info(GAME_INFO_DOUBLE_BOOSTER_DATA_ID, 736, 64);
	register_greedy_info(GAME_INFO_X2_DATA_ID,             768, 64);
	register_greedy_info(GAME_INFO_COMPLETE_DATA_ID,       832, 64);
	register_greedy_info(GAME_INFO_INVINCIBLE_DATA_ID,     864, 64);
	register_greedy_info(GAME_INFO_EMPTY_DATA_ID,          512,  0);

	register_greedy_bonus(JEWELS, 0,   96, 7, 150000);
	register_greedy_bonus(JEWEL1, 32,  32, 2, 500000);
	register_greedy_bonus(JEWEL2, 96,  32, 2, 500000);
	register_greedy_bonus(JEWEL3, 160, 32, 2, 500000);
	register_greedy_bonus(JEWEL4, 224, 32, 2, 500000);
	register_greedy_bonus(JEWEL5, 288, 32, 2, 500000);
	register_greedy_bonus(JEWEL6, 352, 32, 2, 500000);
	register_greedy_bonus(JEWEL7, 416, 32, 2, 500000);
	register_greedy_bonus(SINGLE_BOOSTER, 608, 32, 2, 500000);
	register_greedy_bonus(DOUBLE_BOOSTER, 608,  0, 2, 500000);
	register_greedy_bonus(SUPER_PACGUM, 672, 32, 2, 500000);
	register_greedy_bonus(LIFE, 736, 0, 2, 500000);
	register_greedy_bonus(PACMAN_SLOW, 224, 96, 2, 500000);
	register_greedy_bonus(PACMAN_FAST, 352, 96, 2, 500000);
	register_greedy_bonus(SHIELD, 864, 0, 2, 500000);
	register_greedy_bonus(DIET, 800, 0, 2, 500000);
	register_greedy_bonus(DEATH, 736, 32, 2, 500000);
	register_greedy_bonus(GHOSTS_BANQUET, 800, 32, 2, 500000);
	register_greedy_bonus(GHOSTS_WIPEOUT, 864, 32, 2, 500000);
	register_greedy_bonus(GHOSTS_SLOW, 928, 0, 2, 500000);
	register_greedy_bonus(GHOSTS_FAST, 928, 32, 2, 500000);
	register_greedy_bonus(LOCKDOWN, 672, 0, 2, 500000);
	register_greedy_bonus(X2, 416, 96, 2, 500000);
	register_greedy_bonus(REWIND, 544, 0, 2, 500000);
	register_greedy_bonus(FORWARD, 544, 32, 2, 500000);
	register_greedy_bonus(CASINO, 0, 0, 16, 75000);
	register_greedy_bonus(SURPRISE, 480, 32, 2, 500000);

	register_greedy_jewel(1);
	register_greedy_jewel(2);
	register_greedy_jewel(3);
	register_greedy_jewel(4);
	register_greedy_jewel(5);
	register_greedy_jewel(6);
	register_greedy_jewel(7);

	register_greedy_single(GAME_LIFE_ICON_DATA_ID, GREEDY_GAME_SPRITE,
			       384, 272, 16, 16);
	register_greedy_single(GAME_SHIELD_ICON_DATA_ID, GREEDY_GAME_SPRITE,
			       400, 272, 16, 16);

	register_greedy_single(GAME_PANEL_DATA_ID, GREEDY_GAME_PANEL,
			       0, 0, 640, 80);
	register_greedy_single(GAME_BOTTOM_DATA_ID("left"), GREEDY_GAME_SPRITE,
			       400, 268, 4, 4);
	register_greedy_single(GAME_BOTTOM_DATA_ID("right"), GREEDY_GAME_SPRITE,
			       400, 264, 4, 4);
	register_greedy_single(GAME_PACGUM_DATA_ID, GREEDY_GAME_SPRITE,
			       384, 264, 8, 8);

	static struct greedy_image_data super_pacgum_data;
	static unsigned short int super_pacgum_x[] = { 384, 400 };
	static unsigned short int super_pacgum_y[] = { 248, 248 };
	register_greedy_image_data(
		&super_pacgum_data,
		IMAGE_DATA_ID("greedy", GAME_SUPER_PACGUM_DATA_ID),
		GREEDY_GAME_SPRITE, &greedy_image_ops,
		super_pacgum_x, super_pacgum_y, 16, 16, 2, 500000);

	static struct greedy_image_data layout_data;
	static struct rgba layout_rgba;
	register_greedy_image_data(
		&layout_data, IMAGE_DATA_ID("greedy", GAME_LAYOUT_DATA_ID),
		GREEDY_GAME_PATTERN, &greedy_layout_ops, NULL, NULL,
	       	0, 0, 0, 0);
	layout_data.image_data.rgba = &layout_rgba;

	static struct greedy_image_data pacman_wins_data;
	static unsigned short int pacman_wins_x[20], pacman_wins_y[20];
	register_greedy_image_data(
		&pacman_wins_data,
		IMAGE_DATA_ID("greedy", GAME_PACMAN_WINS_DATA_ID),
		GREEDY_GAME_ANIMATION, &greedy_image_ops, pacman_wins_x,
		pacman_wins_y, 32, 32, b6_card_of(pacman_wins_x), 75000);
	setup_greedy_image_xy(&pacman_wins_data, 0, 64, 1, 0);

	static struct greedy_image_data pacman_loses_data;
	static unsigned short int pacman_loses_x[16], pacman_loses_y[16];
	register_greedy_image_data(
		&pacman_loses_data,
		IMAGE_DATA_ID("greedy", GAME_PACMAN_LOSES_DATA_ID),
		GREEDY_GAME_ANIMATION, &greedy_image_ops, pacman_loses_x,
		pacman_loses_y, 32, 32, b6_card_of(pacman_loses_x), 75000);
	setup_greedy_image_xy(&pacman_loses_data, 0, 32, 1, 0);
	return 0;
}
register_init(greedy_skin_ctor);
