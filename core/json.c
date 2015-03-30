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

#include "json.h"

#include <b6/json.h>
#include "lib/embedded.h"
#include "lib/io.h"
#include "lib/std.h"

static struct json {
	unsigned int refcount;
	struct b6_json json;
	struct b6_json_default_impl impl;
} json = { .refcount = 0, };

struct b6_json *get_json(void)
{
	if (json.refcount++)
		goto done;
	b6_json_default_impl_initialize(&json.impl, &b6_std_allocator);
	b6_json_initialize(&json.json, &json.impl.up, &b6_std_allocator);
done:
	return &json.json;
}

void put_json(struct b6_json *self)
{
	if (--json.refcount)
		return;
	b6_json_default_impl_finalize(&json.impl);
	b6_json_finalize(&json.json);
}

static long int istream_json_read(struct b6_json_istream *up, void *buf,
				  unsigned long int len)
{
	struct json_istream *self = b6_cast_of(up, struct json_istream, up);
	return read_istream(self->istream, buf, len);
}

void setup_json_istream(struct json_istream *self, struct istream *istream)
{
	static const struct b6_json_istream_ops ops = {
		.read = istream_json_read,
	};
	b6_json_setup_istream(&self->up, &ops);
	self->istream = istream;
}

static long int json_ostream_write(struct b6_json_ostream *up, const void *buf,
				   unsigned long int len)
{
	struct json_ostream *self = b6_cast_of(up, struct json_ostream, up);
	return write_ostream(self->ostream, buf, len);
}

static int json_ostream_flush(struct b6_json_ostream *up)
{
	struct json_ostream *self = b6_cast_of(up, struct json_ostream, up);
	return flush_ostream(self->ostream);
}

void initialize_json_ostream(struct json_ostream *self, struct ostream *ostream)
{
	static const struct b6_json_ostream_ops ops = {
		.write = json_ostream_write,
		.flush = json_ostream_flush,
	};
	b6_json_setup_ostream(&self->up, &ops);
	self->ostream = ostream;
}

void finalize_json_ostream(struct json_ostream *self)
{
	flush_ostream(self->ostream);
}

struct b6_json_object *walk_json(struct b6_json_object *self,
				 const struct b6_utf8 *paths,
				 unsigned long int npaths,
				 int create)
{
	for (; npaths--; paths += 1) {
		struct b6_json_value *value = b6_json_get_object(self, paths);
		if (!value) {
			struct b6_json_object *object;
			enum b6_json_error error;
			if (!create)
				return NULL;
			if (!(object = b6_json_new_object(self->json)))
				return NULL;
			value = &object->up;
			if ((error = b6_json_set_object(self, paths, value))) {
				b6_json_unref_value(value);
				return NULL;
			}
			self = object;
			continue;
		}
		if (!(self = b6_json_value_as_or_null(value, object)))
			return NULL;
	}
	return self;
}
