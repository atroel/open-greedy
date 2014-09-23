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

#include "utils.h"

#include "core/rgba.h"
#include "lib/embedded.h"
#include "lib/io.h"
#include "lib/log.h"
#include "lib/std.h"

#include <b6/list.h>

struct lru_cache_entry {
	struct b6_entry entry;
	struct b6_dref dref;
	void *userdata;
};

struct lru_cache {
	void (*dispose)(void*);
	struct b6_registry registry;
	struct b6_list free;
	struct b6_list used;
};

void setup_lru_cache(struct lru_cache *self, void (*dispose)(void*),
		     struct lru_cache_entry *entries, unsigned int nentries)
{
	b6_setup_registry(&self->registry);
	b6_list_initialize(&self->used);
	b6_list_initialize(&self->free);
	self->dispose = dispose;
	while (nentries--)
		b6_list_add_last(&self->free, &entries++->dref);
}

void *get_lru_cache(struct lru_cache *self, const char *name)
{
	struct lru_cache_entry *lru_cache_entry;
	struct b6_entry *entry;
	if (!(entry = b6_lookup_registry(&self->registry, name)))
		return NULL;
	lru_cache_entry = b6_cast_of(entry, struct lru_cache_entry, entry);
	if (&lru_cache_entry->dref != b6_list_last(&self->used)) {
		b6_list_del(&lru_cache_entry->dref);
		b6_list_add_last(&self->used, &lru_cache_entry->dref);
	}
	return lru_cache_entry->userdata;
}

void put_lru_cache(struct lru_cache *self, const char *name, void *userdata)
{
	struct lru_cache_entry *cache_entry;
	struct b6_entry *entry;
	if ((entry = b6_lookup_registry(&self->registry, name))) {
		cache_entry = b6_cast_of(entry, struct lru_cache_entry, entry);
		b6_unregister(&self->registry, &cache_entry->entry);
		self->dispose(cache_entry->userdata);
	} else {
		struct b6_dref *dref = b6_list_first(&self->free);
		if (dref == b6_list_tail(&self->free)) {
			cache_entry = b6_cast_of(b6_list_first(&self->used),
						 struct lru_cache_entry, dref);
			b6_unregister(&self->registry, &cache_entry->entry);
			self->dispose(cache_entry->userdata);
		} else
			cache_entry =
				b6_cast_of(dref, struct lru_cache_entry, dref);
	}
	b6_list_del(&cache_entry->dref);
	b6_list_add_last(&self->used, &cache_entry->dref);
}

struct data_entry_inflater {
	struct ibstream ibs;
	struct izstream izs;
	unsigned char buf[2048];
};

static int initialize_inflater(struct data_entry_inflater *self,
			       const void *buf, unsigned int len)
{
	initialize_ibstream(&self->ibs, buf, len);
	return initialize_izstream(&self->izs, &self->ibs.istream, &self->buf,
				   sizeof(self->buf));
}

static void finalize_inflater(struct data_entry_inflater *self)
{
	finalize_izstream(&self->izs);
}

static void put_buffered_data_entry(struct data_entry *up, struct istream *is)
{
	struct izstream *izs = b6_cast_of(is, struct izstream, up);
	struct data_entry_inflater *inflater = b6_cast_of(
		izs, struct data_entry_inflater, izs);
	finalize_inflater(inflater);
	b6_deallocate(&b6_std_allocator, inflater);
}

static struct istream *get_buffered_data_entry(struct data_entry *up)
{
	struct buffered_data_entry *self =
		b6_cast_of(up, struct buffered_data_entry, up);
	struct data_entry_inflater *inflater =
		b6_allocate(&b6_std_allocator, sizeof(*inflater));
	if (!inflater)
		return NULL;
	if (initialize_inflater(inflater, self->buf, self->len)) {
		put_buffered_data_entry(up, &inflater->izs.up);
		return NULL;
	}
	return &inflater->izs.up;
}

int register_buffered_data(struct buffered_data_entry *self, const void *buf,
			   unsigned int len, const char *name)
{
	static const struct data_entry_ops ops = {
		.get = get_buffered_data_entry,
		.put = put_buffered_data_entry,
	};
	self->buf = buf;
	self->len = len;
	return register_data(&self->up, &ops, name);
}

void unregister_buffered_data(struct buffered_data_entry *self)
{
	unregister_data(&self->up);
}

static struct istream *get_cached_image_data_entry(struct data_entry *up)
{
	struct buffered_data_entry *buffered_data_entry =
		b6_cast_of(up, struct buffered_data_entry, up);
	struct cached_image_data *self = b6_cast_of(buffered_data_entry,
						    struct cached_image_data,
						    buffered_data_entry);
	return &self->image_data.up;
}

static B6_LIST_DEFINE(image_cache);
static unsigned int cache_count = 0;
static unsigned int cache_limit = 2;

static int cached_image_data_ctor(struct image_data *up, void *param)
{
	struct cached_image_data *self =
		b6_cast_of(up, struct cached_image_data, image_data);
	struct data_entry_inflater inflater;
	int retval;
	if (self->shared_rgba.count || self->dref.ref[0]) {
		b6_list_del(&self->dref);
		goto done;
	}
	if (cache_count >= cache_limit) {
		struct b6_dref *dref = b6_list_last(&image_cache);
		for (dref = b6_list_last(&image_cache);
		     dref != b6_list_head(&image_cache);
		     dref = b6_list_walk(dref, B6_PREV)) {
			struct cached_image_data *curr = b6_cast_of(
				dref, struct cached_image_data, dref);
			if (!curr->shared_rgba.count) {
				finalize_rgba(&curr->shared_rgba.rgba);
				curr->image_data.rgba = NULL;
				b6_list_del(dref);
				curr->dref.ref[0] = NULL;
				cache_count -= 1;
				break;
			}
		}
		if (dref == b6_list_head(&image_cache))
			return -1;
	}
	if (initialize_inflater(&inflater, self->buffered_data_entry.buf,
				self->buffered_data_entry.len))
		return -1;
	retval = initialize_rgba_from_tga(&self->shared_rgba.rgba,
					  &inflater.izs.up);
	finalize_inflater(&inflater);
	if (retval) {
		log_e("inflating %s failed with error %d",
		      self->buffered_data_entry.up.entry.name, retval);
		return -1;
	}
	self->image_data.rgba = &self->shared_rgba.rgba;
	self->image_data.w = self->shared_rgba.rgba.w;
	self->image_data.h = self->shared_rgba.rgba.h;
	cache_count += 1;
done:
	self->shared_rgba.count += 1;
	b6_list_add_first(&image_cache, &self->dref);
	return 0;
}

static void cached_image_data_dtor(struct image_data *up)
{
	struct cached_image_data *self =
		b6_cast_of(up, struct cached_image_data, image_data);
	self->shared_rgba.count -= 1;
}

int register_cached_image_data(struct cached_image_data *self,
			       const void *buf, unsigned int len,
			       const char *name)
{
	const static struct data_entry_ops entry_ops = {
		.get = get_cached_image_data_entry,
	};
	const static struct image_data_ops image_ops = {
		.ctor = cached_image_data_ctor,
		.dtor = cached_image_data_dtor,
	};
	static unsigned short int zero = 0;
	self->shared_rgba.count = 0;
	self->buffered_data_entry.buf = buf;
	self->buffered_data_entry.len = len;
	setup_image_data(&self->image_data, &image_ops, NULL,
			 &zero, &zero, 0, 0, 1, 0);
	self->dref.ref[0] = NULL;
	return register_data(&self->buffered_data_entry.up, &entry_ops, name);
}

void unregister_cached_image_data(struct cached_image_data *self)
{
	unregister_data(&self->buffered_data_entry.up);
}

static int copy_data_path(char **buf, unsigned long int *len, const char *ptr)
{
	while (*ptr) {
		if (!*len)
			return -1;
		**buf = *ptr++;
		*len -= 1;
		*buf += 1;
	}
	return 0;
}

static int make_external_data_path(const char *path,
				   char *buf, unsigned long int len)
{
	static const char separator[] = "/";
	int retval;
	if (!len)
		return -1;
	len -= 1;
	if ((retval = copy_data_path(&buf, &len, get_data_path())))
		return retval;
	if (buf[-1] != *separator &&
	    (retval = copy_data_path(&buf, &len, separator)))
			return retval;
	if ((retval = copy_data_path(&buf, &len, path)))
		return retval;
	*buf++ = '\0';
	return retval;
}

int load_external_data(struct membuf *self, const char *subpath)
{
	struct ifstream ifstream;
	int retval;
	long long int length;
	char path[512];
	if ((retval = make_external_data_path(subpath, path, sizeof(path)))) {
		log_e("path is too long: %s/%s", get_data_path(), subpath);
		return retval;
	}
	if ((retval = initialize_ifstream(&ifstream, path))) {
		log_e("cannot open %s", path);
		return retval;
	}
	length = fill_membuf(self, &ifstream.istream);
	if (length < 0) {
		log_e("cannot read %s", path);
		retval = length;
	}
	finalize_ifstream(&ifstream);
	return retval;
}
