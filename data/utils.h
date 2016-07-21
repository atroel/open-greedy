/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014-2016 Arnaud TROEL
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

#ifndef UTILS_H
#define UTILS_H

#include "core/rgba.h"
#include "core/data.h"

#include <b6/refs.h>

struct buffered_data_entry {
	struct data_entry up;
	const void *buf;
	unsigned int len;
};

struct shared_rgba {
	struct rgba rgba;
	unsigned int count;
};

struct cached_image_data {
	struct buffered_data_entry buffered_data_entry;
	struct image_data image_data;
	struct b6_dref dref;
	struct shared_rgba shared_rgba;
};

extern int register_buffered_data(struct buffered_data_entry *self,
				  const void *buf, unsigned int len,
				  const struct b6_utf8 *id);

extern void unregister_buffered_data(struct buffered_data_entry *self);

extern int register_cached_image_data(struct cached_image_data *self,
				      const void *buf, unsigned int len,
				      const struct b6_utf8 *id);

void unregister_cached_image_data(struct cached_image_data *self);

extern int load_external_data(struct membuf *self, const char *path);

#endif /* UTILS_H */
