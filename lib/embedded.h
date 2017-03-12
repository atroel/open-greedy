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

#ifndef EMBEDDED_H
#define EMBEDDED_H

#include <b6/registry.h>
#include "lib/io.h"

#define publish_embedded(_data, _size, _id) \
	static struct embedded embedded_ ## _data; \
	b6_ctor(publish_embedded_ ## _data); \
	static void publish_embedded_ ## _data(void) \
	{ \
		struct embedded *self = &embedded_ ## _data; \
		self->buf = _data; \
		self->len = sizeof(_data); \
		self->uncompressed_len = _size; \
		register_embedded(self, _id); \
	} \
	static struct embedded embedded_ ## _data

struct embedded {
	struct b6_entry entry;
	const void *buf;
	unsigned long int len;
	unsigned long int uncompressed_len;
};

extern struct b6_registry __embedded_registry;

static inline int register_embedded(struct embedded *self,
				    const struct b6_utf8 *id)
{
	return b6_register(&__embedded_registry, &self->entry, id);
}

static inline void unregister_embedded(struct embedded *self)
{
	b6_unregister(&__embedded_registry, &self->entry);
}

static inline struct embedded *lookup_embedded(const struct b6_utf8 *id)
{
	struct b6_entry *entry = b6_lookup_registry(&__embedded_registry, id);
	return entry ? b6_cast_of(entry, struct embedded, entry) : NULL;
}

#endif /* EMBEDDED_H */
