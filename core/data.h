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

#ifndef SKIN_H
#define SKIN_H

#include "env.h"
#include "items.h"
#include "level.h"
#include <b6/registry.h>
#include <b6/utf8.h>

extern const char *level_data_type;
extern const char *audio_data_type;
extern const char *image_data_type;

#define __DATA_ID(skin_id, type_id, data_id) (skin_id "." type_id "." data_id)
#define DATA_ID(skin_id, type_id, data_id) __DATA_ID(skin_id, type_id, data_id)
#define LEVEL_DATA_ID(skin_id, data_id) DATA_ID(skin_id, "level", data_id)
#define AUDIO_DATA_ID(skin_id, data_id) DATA_ID(skin_id, "audio", data_id)
#define IMAGE_DATA_ID(skin_id, data_id) DATA_ID(skin_id, "image", data_id)

extern const char *get_skin_id(void);

static inline const char *get_data_path(void) { return get_ro_dir(); }

struct data_entry {
	struct b6_entry entry;
	const struct data_entry_ops *ops;
};

struct data_entry_ops {
	struct istream *(*get)(struct data_entry*);
	void (*put)(struct data_entry*, struct istream*);
};

extern struct b6_registry __data_registry;

static inline int register_data(struct data_entry *self,
				const struct data_entry_ops *ops,
				const struct b6_utf8 *name)
{
	self->ops = ops;
	return b6_register(&__data_registry, &self->entry, name);
}

static inline void unregister_data(struct data_entry *self)
{
	b6_unregister(&__data_registry, &self->entry);
}

extern struct data_entry *lookup_data(const char *skin, const char *type,
				      const char *what);

static inline struct istream *get_data(struct data_entry *entry)
{
	return entry->ops->get(entry);
}

static inline void put_data(struct data_entry *entry, struct istream *istream)
{
	if (entry->ops->put)
		entry->ops->put(entry, istream);
}

struct image_data {
	struct istream up;
	const struct image_data_ops *ops;
	const struct rgba *rgba;
	unsigned short int *x;
	unsigned short int *y;
	unsigned short int w;
	unsigned short int h;
	unsigned int length;
	unsigned long long int period;
};

struct image_data_ops {
	int (*ctor)(struct image_data*, void*);
	void (*dtor)(struct image_data*);
};

static inline void setup_image_data(struct image_data *self,
				    const struct image_data_ops *ops,
				    struct rgba *rgba,
				    unsigned short int *x,
				    unsigned short int *y,
				    unsigned short int w,
				    unsigned short int h,
				    unsigned int length,
				    unsigned long long int period)
{
	self->up.ops = NULL;
	self->ops = ops;
	self->rgba = rgba;
	self->x = x;
	self->y = y;
	self->w = w;
	self->h = h;
	self->length = length;
	self->period = period;
}

extern int get_image_data_no_fallback(const char *skin_id, const char *data_id,
				      void *param, struct data_entry **entry,
				      struct image_data **data);

extern int get_image_data(const char *skin_id, const char *data_id,
			  void *param, struct data_entry **entry,
			   struct image_data **data);

extern void put_image_data(struct data_entry *entry, struct image_data *data);

#endif /* SKIN_H */
