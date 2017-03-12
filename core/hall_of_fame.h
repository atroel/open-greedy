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

#ifndef HALL_OF_FAME_H
#define HALL_OF_FAME_H

#include <b6/allocator.h>
#include <b6/json.h>
#include <b6/list.h>
#include <b6/registry.h>
#include <b6/utf8.h>

struct hall_of_fame {
	char path_buffer[512];
	struct b6_fixed_allocator path_allocator;
	struct b6_utf8_string path;
	struct b6_json_object *object;
};

extern int initialize_hall_of_fame(struct hall_of_fame *self,
				   struct b6_json *json, const char *path);

extern void finalize_hall_of_fame(struct hall_of_fame *self);

extern int load_hall_of_fame(struct hall_of_fame *self);

extern int save_hall_of_fame(const struct hall_of_fame *self);

extern struct b6_json_array *open_hall_of_fame(struct hall_of_fame *self,
					       const char *levels,
					       const struct b6_utf8 *config);

extern void close_hall_of_fame(struct b6_json_array *array);

extern struct b6_json_object *amend_hall_of_fame(struct b6_json_array *array,
						 unsigned long int level,
						 unsigned long int score);

struct hall_of_fame_iterator {
	struct b6_json_array *a;
	unsigned int i;
};

static inline void reset_hall_of_fame_iterator(
	struct hall_of_fame_iterator *self, struct b6_json_array *array)
{
	self->a = array;
	self->i = 0;
}

static inline int hall_of_fame_iterator_has_next(
	const struct hall_of_fame_iterator *self) {
	return self->i < b6_json_array_len(self->a);
}

static inline struct b6_json_object *hall_of_fame_iterator_get_next(
	struct hall_of_fame_iterator *self)
{
	return b6_json_get_array_as(self->a, self->i++, object);
}

extern int split_hall_of_fame_entry(struct b6_json_object *object,
				    unsigned int *level, unsigned int *score,
				    const struct b6_utf8 **label);

extern int alter_hall_of_fame_entry(struct b6_json_object *object,
				    const struct b6_utf8* label);

#endif /* HALL_OF_FAME_H */
