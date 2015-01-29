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

#include "items.h"
#include "lib/rng.h"

#define notify_item_observers(_item, _mobile, _op) \
	do { \
		struct item *_i = (_item); \
		struct mobile *_m = (_mobile); \
		b6_notify_observers(&_i->observers, item_observer, _op, _m, \
				   _i); \
	} while (0)

void pacman_visits_item(struct mobile *m, struct item *i)
{
	notify_item_observers(i, m, on_pacman_visit);
}

void ghost_visits_item(struct mobile *m, struct item *i)
{
	notify_item_observers(i, m, on_ghost_visit);
}

static void initialize_item(struct item *self, void (*dispose)(struct item*))
{
	b6_list_initialize(&self->observers);
	self->dispose = dispose;
}

static void dispose_pacgum(struct item *item)
{
	b6_cast_of(item, struct pacgum, item)->count -= 1;
}

static void dispose_super_pacgum(struct item *item)
{
	b6_cast_of(item, struct super_pacgum, item)->count -= 1;
}

struct item *clone_bonus(struct items *self)
{
	self->bonus.contents = read_random_number_generator() * BONUS_COUNT;
	return &self->bonus.item;
}

enum bonus_type unveil_bonus_contents(struct item *bonus_as_item)
{
	struct bonus *self = b6_cast_of(bonus_as_item, struct bonus, item);
	if (self->contents == SURPRISE_BONUS)
		self->contents =
			read_random_number_generator() * (BONUS_COUNT - 1);
	return self->contents;
}

static void dispose_teleport(struct item *item)
{
	b6_cast_of(item, struct teleport, item)->place = NULL;
}

void initialize_items(struct items *self)
{
	initialize_item(&self->empty, NULL);
	initialize_item(&self->pacgum.item, dispose_pacgum);
	self->pacgum.count = 0;
	initialize_item(&self->super_pacgum.item, dispose_super_pacgum);
	self->super_pacgum.count = 0;
	initialize_item(&self->bonus.item, NULL);
	initialize_item(&self->teleport[0].item, dispose_teleport);
	initialize_item(&self->teleport[1].item, dispose_teleport);
	self->teleport[0].teleport = &self->teleport[1];
	self->teleport[0].place = NULL;
	self->teleport[1].teleport = &self->teleport[0];
	self->teleport[1].place = NULL;
}
