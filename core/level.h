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

#ifndef LEVEL_H
#define LEVEL_H

#include "b6/assert.h"
#include "b6/registry.h"

#include "lib/io.h"
#include "items.h"

#define LEVEL_WIDTH 40
#define LEVEL_HEIGHT 25

enum {
	LAYOUT_WALL          =  0,
	LAYOUT_PAC_GUM_1     = 17,
	LAYOUT_PAC_GUM_2     = 18,
	LAYOUT_SUPER_PAC_GUM = 19,
	LAYOUT_BONUS         = 20,
	LAYOUT_PACMAN        = 21,
	LAYOUT_GHOSTS        = 22,
	LAYOUT_EMPTY         = 23,
	LAYOUT_TELEPORT      = 24,
};

struct layout {
	unsigned char data[LEVEL_WIDTH + 2][LEVEL_HEIGHT + 2];
};

static inline int get_layout(const struct layout *l, int x, int y)
{
	x += 1;
	y += 1;
	b6_precond(x >= 0);
	b6_precond(x <= LEVEL_WIDTH + 1);
	b6_precond(y >= 0);
	b6_precond(y <= LEVEL_HEIGHT + 1);
	return l->data[x][y];
}

static inline int serialize_layout(const struct layout *l, struct ostream *s)
{
	if (write_ostream(s, l->data, sizeof(l->data)) < sizeof(l->data))
		return -1;
	return 0;
}

static inline int unserialize_layout(struct layout *l, struct istream *s)
{
	if (read_istream(s, l->data, sizeof(l->data)) < sizeof(l->data))
		return -1;
	return 0;
}

struct layout_debug_ostream {
	struct ostream ostream;
	unsigned short int x, y;
	struct ostream *o;
};

extern const struct ostream_ops __layout_debug_ostream_ops;

static inline void initialize_layout_debug_ostream(
	struct layout_debug_ostream *s, struct ostream* o)
{
	s->ostream.ops = &__layout_debug_ostream_ops;
	s->x = 0;
	s->y = 0;
	s->o = o;
}

struct mobile;
struct pacman;
struct ghost;
struct place;
struct level;

struct place {
	struct item *item;
};

enum direction {
	LEVEL_E = 0,
	LEVEL_W = 1,
	LEVEL_S = 2,
	LEVEL_N = 3,
};

#define for_each_direction(_d) \
	(void)(&(_d) == (enum direction*)NULL); \
	for (_d = LEVEL_E; _d <= LEVEL_N; _d += 1)

static inline int get_opposite(enum direction d) { return d ^ 1; }

static inline int is_opposite(enum direction lhs, enum direction rhs)
{
	return lhs == get_opposite(rhs);
}

struct level {
	struct place *pacman_home;
	struct place *ghosts_home;
	struct place *bonus_place;
	struct place places[LEVEL_WIDTH*LEVEL_HEIGHT];
	unsigned int nplaces;
	struct items *items;
};

static inline void place_location(const struct level *l, const struct place *p,
				  int *x, int *y)
{
	int offset = p - l->places;
	*x = offset % LEVEL_WIDTH;
	*y = offset / LEVEL_WIDTH;
}

extern struct place *place_neighbor(struct level*, struct place*,
				    enum direction);

extern struct item *set_level_place_item(struct level*, struct place*,
					 struct item*);

extern int initialize_level(struct level*, struct items*);

extern void finalize_level(struct level*);

static inline int is_level_open(const struct level *self)
{
	return !!self->pacman_home;
}

extern int __open_level(struct level*, struct layout*);

extern void __close_level(struct level*);

static inline int open_level(struct level *self, struct layout *layout)
{
	return is_level_open(self) ? -1 : __open_level(self, layout);
}

static inline int close_level(struct level *self)
{
	if (!is_level_open(self))
		return -1;
	__close_level(self);
	return 0;
}

struct level_iterator {
	struct level *l;
	unsigned long int i;
};

static inline int initialize_level_iterator(struct level_iterator *self,
					    struct level *level)
{
	self->l = level;
	self->i = 0;
	return is_level_open(level) ? 0 : -1;
}

static inline int level_iterator_has_next(struct level_iterator *self)
{
	for (;;) {
		int i = self->i;
		if (self->i >= LEVEL_WIDTH * LEVEL_HEIGHT)
			return 0;
		self->i += 1;
		if (self->l->places[i].item || &self->l->places[i] ==
		    get_teleport_destination(&self->l->items->teleport[1].item))
			return 1;
	}
}

static inline struct place *level_iterator_next(struct level_iterator *self)
{
	return &self->l->places[self->i - 1];
}

#endif /* LEVEL_H */
