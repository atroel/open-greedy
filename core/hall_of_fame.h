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

#ifndef HALL_OF_FAME_H
#define HALL_OF_FAME_H

#include <b6/list.h>
#include <b6/registry.h>

struct hall_of_fame_entry {
	struct b6_dref dref;
	char name[16];
	unsigned int level;
	unsigned int score;
};

struct hall_of_fame {
	struct b6_entry entry;
	struct b6_list list;
	struct hall_of_fame_entry entries[10];
	struct b6_utf8_string name;
};

struct hall_of_fame_iterator {
	const struct b6_list *list;
	struct b6_dref *dref;
};

static inline void setup_hall_of_fame_iterator(
	const struct hall_of_fame *self, struct hall_of_fame_iterator *iter)
{
	iter->list = &self->list;
	iter->dref = b6_list_first(iter->list);
}

static inline struct hall_of_fame_entry *hall_of_fame_iterator_next(
	struct hall_of_fame_iterator *iter)
{
	struct hall_of_fame_entry *entry = NULL;
	if (iter->dref != b6_list_tail(iter->list)) {
		entry = b6_cast_of(iter->dref, struct hall_of_fame_entry, dref);
		iter->dref = b6_list_walk(iter->dref, B6_NEXT);
	}
	return entry;
}

extern struct hall_of_fame *load_hall_of_fame(
	const char *levels_name, const struct b6_utf8 *config_name);

extern int save_hall_of_fame(struct hall_of_fame*);

extern struct hall_of_fame_entry *get_hall_of_fame_entry(
	struct hall_of_fame *self, unsigned long int level,
	unsigned long int score);

extern void put_hall_of_fame_entry(struct hall_of_fame *self,
				   struct hall_of_fame_entry *entry,
				   const char *name);

#endif /* HALL_OF_FAME_H */
