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

#include "level.h"

#include <b6/cmdline.h>

#include "lib/rng.h"
#include "lib/std.h"
#include "lib/log.h"
#include "data.h"
#include "pacman.h"
#include "items.h"

B6_REGISTRY_DEFINE(__layout_provider_registry);

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

int print_layout(const struct layout *layout, struct ostream *ostream)
{
	int x, y;
	char ascii[LEVEL_WIDTH + 3];
	long long int wsize;
	ascii[LEVEL_WIDTH + 2] = '\n';
	for (y = 0; y < LEVEL_HEIGHT + 2; y += 1) {
		for (x = 0; x < LEVEL_WIDTH + 2; x += 1)
			ascii[x] = layout_to_char(layout->data[x][y]);
		wsize = write_ostream(ostream, ascii, sizeof(ascii));
		if (wsize < 0) {
			log_e("i/o error #%d", (int)wsize);
			return -1;
		}
		if (wsize < sizeof(ascii)) {
			log_e("truncated file");
			return -1;
		}
	}
	return 0;
}

int serialize_layout(const struct layout *l, struct ostream *s)
{
	if (write_ostream(s, l->data, sizeof(l->data)) < sizeof(l->data))
		return -1;
	return 0;
}

static void frame_layout(struct layout *layout) {
	int x, y;
	for (x = 0; x < LEVEL_WIDTH + 2; x += 1) {
		layout->data[x][0] = LAYOUT_WALL;
		layout->data[x][LEVEL_HEIGHT + 1] = LAYOUT_WALL;
	}
	for (y = 1; y < LEVEL_HEIGHT + 1; y += 1) {
		layout->data[LEVEL_WIDTH + 1][y] = LAYOUT_WALL;
		layout->data[0][y] = LAYOUT_WALL;
	}
}

static unsigned char char_to_layout(char c)
{
	switch (c) {
	case '.': return LAYOUT_PAC_GUM_1;
	case ',': return LAYOUT_PAC_GUM_2;
	case '*': return LAYOUT_SUPER_PAC_GUM;
	case 'b': return LAYOUT_BONUS;
	case 'p': return LAYOUT_PACMAN;
	case 'g': return LAYOUT_GHOSTS;
	case ' ': return LAYOUT_EMPTY;
	case 't': return LAYOUT_TELEPORT;
	case '@':
	default:
		  return LAYOUT_WALL;
	}
}

int parse_layout(struct layout *layout, struct istream *istream)
{
	int x, y;
	char ascii[LEVEL_WIDTH + 2];
	long long int rsize;
	for (y = 0; y < LEVEL_HEIGHT + 2; y += 1) {
		rsize = read_istream(istream, ascii, sizeof(ascii));
		if (rsize < 0) {
			log_e("i/o error #d", (int)rsize);
			return -1;
		}
		if (rsize < sizeof(ascii)) {
			log_e("truncated stream");
			return -1;
		}
		for (x = 0; x < b6_card_of(ascii); x += 1)
			layout->data[x][y] = char_to_layout(ascii[x]);
		do
			rsize = read_istream(istream, ascii, 1);
		while (rsize > 0 && *ascii != '\n');
	}
	frame_layout(layout);
	return 0;
}

int unserialize_layout(struct layout *l, struct istream *s)
{
	if (read_istream(s, l->data, sizeof(l->data)) < sizeof(l->data))
		return -1;
	frame_layout(l);
	return 0;
}

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
	for (i = 0; i < b6_card_of(self->places); i += 1) {
		struct place *place = &self->places[i];
		initialize_place(self, layout, place);
		self->nplaces += !!place->item;
	}
	self->teleport_places[0] = self->items->teleport[0].place;
	self->teleport_places[1] = self->items->teleport[1].place;
	if (self->teleport_places[0] && self->teleport_places[1]) {
		self->items->teleport[0].place = self->teleport_places[1];
		self->items->teleport[1].place = self->teleport_places[0];
		if (self->teleport_places[0]->item !=
		    &self->items->teleport[0].item)
			dispose_item(&self->items->teleport[0].item);
		if (self->teleport_places[1]->item !=
		    &self->items->teleport[1].item)
			dispose_item(&self->items->teleport[1].item);
	} else if (self->teleport_places[0]) {
		int x, y;
		place_location(self, self->teleport_places[0], &x, &y);
		log_w("ignored single teleport at (%d,%d).", x, y);
		dispose_item(self->teleport_places[0]->item);
		self->teleport_places[0]->item = clone_empty(self->items);
		self->teleport_places[0] = NULL;
	}
	b6_check(!self->teleport_places[1] ||
		 (self->teleport_places[1] && self->teleport_places[0]));
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

static struct data_entry *lookup_data_layout(const char *id, unsigned int n)
{
	char level[] = "00";
	level[0] += n / 10;
	level[1] += n % 10;
	return n > 99 ? NULL : lookup_data(id, level_data_type, level);
}

static int data_layout_provider_get(struct layout_provider *up, unsigned int n,
				    struct layout *layout)
{
	struct data_entry *entry;
	struct istream *istream;
	int retval;
	if (!(entry = lookup_data_layout(up->id, n))) {
		log_w("could not find %s level #%d", up->id, n);
		return -1;
	}
	if (!(istream = get_data(entry))) {
		log_w("out of memory when reading %s level #%d", up->id, n);
		return -1;
	}
	if ((retval = unserialize_layout(layout, istream)))
		log_w("error %d when reading %s level #%d", up->id, retval, n);
	put_data(entry, istream);
	return retval;
}

static void reset_layout_provider(struct layout_provider *self,
				  const struct layout_provider_ops *ops,
				  const char *id, unsigned int size)
{
	self->ops = ops;
	self->id = id;
	self->size = size;
}

int reset_data_layout_provider(struct data_layout_provider *self,
			       const char *id)
{
	static const struct layout_provider_ops ops = {
		.get = data_layout_provider_get,
	};
	unsigned int size;
	for (size = 0; lookup_data_layout(id, size + 1); size += 1);
	if (!size)
		return -1;
	reset_layout_provider(&self->up, &ops, id, size);
	return 0;
}

static int layout_shuffler_get(struct layout_provider *up, unsigned int n,
			       struct layout *layout)
{
	struct layout_shuffler *self =
		b6_cast_of(up, struct layout_shuffler, up);
	return get_layout_from_provider(self->layout_provider,
					1 + self->index[n - 1], layout);
}

void reset_layout_shuffler(struct layout_shuffler *self,
			   struct layout_provider *layout_provider)
{
	static const struct layout_provider_ops ops = {
		.get = layout_shuffler_get,
	};
	unsigned int i;
	reset_layout_provider(&self->up, &ops, layout_provider->id,
			      layout_provider->size);
	self->layout_provider = layout_provider;
	for (i = 0; i < layout_provider->size; i += 1)
		self->index[i] = i;
	for (i = 0 ; i < 10000; i += 1) {
		unsigned int a = read_random_number_generator() *
			layout_provider->size;
		unsigned int b = read_random_number_generator() *
			layout_provider->size;
		unsigned int index = self->index[a];
		self->index[a] = self->index[b];
		self->index[b] = index;
	}
}
