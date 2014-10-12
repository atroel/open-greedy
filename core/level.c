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

#include "level.h"

#include <b6/cmdline.h>

#include "lib/std.h"
#include "lib/log.h"
#include "pacman.h"
#include "items.h"

static unsigned int ghost_den_recovery_radius = 2;
b6_flag(ghost_den_recovery_radius, uint);

static char layout_to_char(int code)
{
	switch (code) {
	case LAYOUT_WALL:          return '@';
	case LAYOUT_PAC_GUM_1:     return '.';
	case LAYOUT_PAC_GUM_2:     return ',';
	case LAYOUT_SUPER_PAC_GUM: return '*';
	case LAYOUT_BONUS:         return 'b';
	case LAYOUT_PACMAN:        return 'p';
	case LAYOUT_GHOSTS:        return 'g';
	case LAYOUT_EMPTY:         return ' ';
	case LAYOUT_TELEPORT:      return 't';
	}
	return '?';
}

static long long int write_layout_debug(struct ostream *o, const void *b,
					unsigned long long int n)
{
	struct layout_debug_ostream *self =
		b6_cast_of(o, struct layout_debug_ostream, ostream);
	unsigned char buffer[LEVEL_HEIGHT + 2][LEVEL_WIDTH + 3];
	const unsigned char *p;
	int x, y;
	for (p = b, x = 0; x < LEVEL_WIDTH + 2; x += 1)
		for (y = 0; y < LEVEL_HEIGHT + 2; y += 1)
			buffer[y][x] = layout_to_char(*p++);
	for (y = 0; y < LEVEL_HEIGHT + 2; ++y)
		buffer[y][LEVEL_WIDTH + 2] = '\n';
	if (write_ostream(self->o, buffer, sizeof(buffer)) < sizeof(buffer))
		return -1;
	return n;
}

const struct ostream_ops __layout_debug_ostream_ops = {
	.write = write_layout_debug,
};

static struct place *__get_place(struct level *l, int x, int y)
{
	return &l->places[y * LEVEL_WIDTH + x];
}

static struct place *get_place(struct level *l, int x, int y)
{
	struct place *place;
	if (y < 0 || x < 0 || y > LEVEL_HEIGHT || x > LEVEL_WIDTH)
		return NULL;
	place = __get_place(l, x, y);
	if (!place->item)
		return NULL;
	return place;
}

struct place *place_neighbor(struct level *l, struct place *p, enum direction d)
{
	int x, y;
	place_location(l, p, &x, &y);
	switch (d) {
	case LEVEL_N: return get_place(l, x, y - 1);
	case LEVEL_E: return get_place(l, x + 1, y);
	case LEVEL_S: return get_place(l, x, y + 1);
	case LEVEL_W: return get_place(l, x - 1, y);
	}
	return NULL;
}

static int is_place_clear(struct layout *layout, int x, int y)
{
	return get_layout(layout, x, y) != LAYOUT_WALL &&
		get_layout(layout, x, y + 1) != LAYOUT_WALL &&
		get_layout(layout, x + 1, y + 1) != LAYOUT_WALL;
}

static void initialize_place(struct level *level, struct layout *layout,
			     struct place *p)
{
	int x, y, is_clear;
	struct item *item;
	p->item = NULL;
	place_location(level, p, &x, &y);
	is_clear = is_place_clear(layout, x, y);
	switch (get_layout(layout, x + 1, y)) {
	case LAYOUT_SUPER_PAC_GUM:
		if (is_clear)
			p->item = clone_super_pacgum(level->items);
		else
			log_w("no room for super pac gum at (%d,%d).", x, y);
		break;
	case LAYOUT_BONUS:
		if (!is_clear) {
			log_w("no room for bonus at (%d,%d).", x, y);
			break;
		}
		level->bonus_place = p;
		p->item = clone_empty(level->items);
		break;
	case LAYOUT_PACMAN:
		if (!is_clear) {
			log_w("no room for pacman home at (%d,%d).", x, y);
			break;
		}
		p->item = clone_empty(level->items);
		if (level->pacman_home)
			log_w("ignored extra pacman home at (%d,%d).", x, y);
		else
			level->pacman_home = p;
		break;
	case LAYOUT_GHOSTS:
		if (!is_clear)
			log_w("ghosts den at (%d,%d) to be recovered.", x, y);
		p->item = clone_empty(level->items);
		if (level->ghosts_home)
			log_w("ignored extra ghosts den at (%d,%d).", x, y);
		else
			level->ghosts_home = p;
		break;
	case LAYOUT_TELEPORT:
		if (!(item = clone_teleport(level->items, p))) {
			log_w("ignored extra teleport at (%d,%d).", x, y);
			if (is_clear)
				p->item = clone_empty(level->items);
		} else if (is_clear)
			p->item = item;
		break;
	case LAYOUT_PAC_GUM_1:
	case LAYOUT_PAC_GUM_2:
		if (is_clear)
			p->item = clone_pacgum(level->items);
		break;
	case LAYOUT_EMPTY:
		if (is_clear)
			p->item = clone_empty(level->items);
		break;
	case LAYOUT_WALL:
		break;
	}
}

static void mark_level_as_closed(struct level *self)
{
	self->pacman_home = NULL;
	self->ghosts_home = NULL;
	self->bonus_place = NULL;
	self->nplaces = 0;
}

void __close_level(struct level *self)
{
	int i;
	for (i = 0; i < b6_card_of(self->places); i += 1)
		if (self->places[i].item)
			dispose_item(self->places[i].item);
	mark_level_as_closed(self);
}

static void recover_ghosts_den(struct level *self, struct layout *layout)
{
	int xs, ys, xd, yd;
	struct level_iterator iter;
	struct place *closest_place = NULL;
	unsigned int distance, min_distance = LEVEL_WIDTH + LEVEL_HEIGHT;
	if (!self->ghosts_home)
		return;
	place_location(self, self->ghosts_home, &xs, &ys);
	if (is_place_clear(layout, xs, ys))
		return;
	log_i("recovering from non-accessible ghosts den.");
	initialize_level_iterator(&iter, self);
	while (level_iterator_has_next(&iter)) {
		struct place *place = level_iterator_next(&iter);
		place_location(self, place, &xd, &yd);
		distance = (xs > xd ? xs - xd : xd - xs) +
			(ys > yd ? ys - yd : yd - ys);
		if (distance && distance < min_distance) {
			closest_place = place;
			min_distance = distance;
		}
	}
	if (!closest_place) {
		log_e("could not find the closest place to ghost den.");
		return;
	}
	if (min_distance > ghost_den_recovery_radius) {
		log_w("could not find a place close enough to ghost den "
		      "(%u > %u).", min_distance, ghost_den_recovery_radius);
		return;
	}
	place_location(self, closest_place, &xd, &yd);
	while (--min_distance) {
		if (xs < xd)
			xs += 1;
		else if (xs > xd)
			xs -= 1;
		else if (ys < yd)
			ys += 1;
		else if (ys > yd)
			ys -= 1;
		__get_place(self, xs, ys)->item = clone_empty(self->items);
	}
}

int __open_level(struct level *self, struct layout *layout)
{
	int i;
	struct place* places[2];
	for (i = 0; i < b6_card_of(self->places); i += 1) {
		struct place *place = &self->places[i];
		initialize_place(self, layout, place);
		self->nplaces += !!place->item;
	}
	places[0] = self->items->teleport[0].place;
	places[1] = self->items->teleport[1].place;
	if (places[0] && places[1]) {
		self->items->teleport[0].place = places[1];
		self->items->teleport[1].place = places[0];
		if (places[0]->item != &self->items->teleport[0].item)
			dispose_item(&self->items->teleport[0].item);
		if (places[1]->item != &self->items->teleport[1].item)
			dispose_item(&self->items->teleport[1].item);
	} else if (places[0]) {
		int x, y;
		place_location(self, places[0], &x, &y);
		log_w("ignored single teleport at (%d,%d).", x, y);
		dispose_item(places[0]->item);
		places[0]->item = clone_empty(self->items);
	}
	b6_assert(!places[1] || (places[1] && places[0]));
	recover_ghosts_den(self, layout);
	if (self->pacman_home)
		return 0;
	__close_level(self);
	return -1;
}

void finalize_level(struct level *self)
{
	close_level(self);
}

int initialize_level(struct level *self, struct items *items)
{
	self->items = items;
	mark_level_as_closed(self);
	return 0;
}

struct item *set_level_place_item(struct level *self, struct place *place,
				  struct item *new_item)
{
	struct item *old_item = place->item;
	dispose_item(old_item);
	place->item = new_item;
	return old_item;
}
