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

#include "hall_of_fame.h"
#include "env.h"
#include "lib/io.h"
#include "lib/log.h"
#include "lib/std.h"
#include <b6/registry.h>

B6_REGISTRY_DEFINE(hall_of_fame_registry);

struct hall_of_fame_entry *get_hall_of_fame_entry(struct hall_of_fame *self,
						  unsigned long int level,
						  unsigned long int score)
{
	struct hall_of_fame_iterator iter;
	struct hall_of_fame_entry *entry;
	setup_hall_of_fame_iterator(self, &iter);
	while ((entry = hall_of_fame_iterator_next(&iter))) {
		if (score <= entry->score)
			continue;
		if (&entry->dref != b6_list_last(&self->list)) {
			struct b6_dref *r = b6_list_del_last(&self->list);
			b6_list_add(&entry->dref, r);
			entry = b6_cast_of(r, struct hall_of_fame_entry, dref);
		}
		entry->level = level;
		entry->score = score;
		return entry;
	}
	return NULL;
}

void put_hall_of_fame_entry(struct hall_of_fame *self,
			    struct hall_of_fame_entry *entry, const char *name)
{
	int i = 0;
	while (*name && i < b6_card_of(entry->name)) {
		char c = *name++;
		if (c < ' ')
			c = '.';
		entry->name[i++] = c;
	}
	if (i < b6_card_of(entry->name))
		entry->name[i] = '\0';
}

static int read_u32(struct istream *istream, unsigned int *value)
{
	unsigned char buffer[sizeof(*value)];
	int i;
	b6_static_assert(sizeof(buffer) == 4);
	if (read_istream(istream, buffer, sizeof(buffer)) < sizeof(buffer))
		return -1;
	*value = 0;
	for (i = b6_card_of(buffer); i--;)
		*value = *value * 256 + buffer[i];
	return 0;
}

static int read_hall_of_fame_entry(struct hall_of_fame_entry *self,
				   struct istream *istream)
{
       	int retval = read_istream(istream, self->name, sizeof(self->name));
	if (retval < sizeof(self->name))
		return -1;
	if (read_u32(istream, &self->level))
		return -1;
	return read_u32(istream, &self->score);
}

static int read_hall_of_fame(struct hall_of_fame *self, struct istream *istream)
{
	int retval;
	struct hall_of_fame_iterator iter;
	struct hall_of_fame_entry *entry;
	struct izstream izs;
	char zbuf[1024];
	if ((retval = initialize_izstream(&izs, istream, zbuf, sizeof(zbuf))))
		goto fail_zstream;
	setup_hall_of_fame_iterator(self, &iter);
	while ((entry = hall_of_fame_iterator_next(&iter)))
		if ((retval = read_hall_of_fame_entry(entry, &izs.up)))
			break;
	finalize_izstream(&izs);
fail_zstream:
	return retval;
}

static int write_u32(struct ostream *ostream, unsigned int value)
{
	unsigned char buffer[sizeof(value)];
	int i;
	b6_static_assert(sizeof(buffer) == 4);
	for (i = 0; i < b6_card_of(buffer); i += 1) {
		buffer[i] = value & 0xff;
		value /= 256;
	}
	if (write_ostream(ostream, buffer, sizeof(buffer)) < sizeof(buffer))
	       return -1;
	return 0;
}

static int write_hall_of_fame_entry(const struct hall_of_fame_entry *self,
				struct ostream *ostream)
{
	write_ostream(ostream, self->name, sizeof(self->name));
	write_u32(ostream, self->level);
	write_u32(ostream, self->score);
	return 0;
}

static int write_hall_of_fame(const struct hall_of_fame *self,
			      struct ostream *ostream)
{
	int retval = 0;
	struct hall_of_fame_iterator iter;
	const struct hall_of_fame_entry *entry;
	struct ozstream ozs;
	char zbuf[1024];
	if ((retval = initialize_ozstream(&ozs, ostream, zbuf, sizeof(zbuf))))
		goto fail_zstream;
	setup_hall_of_fame_iterator(self, &iter);
	while ((entry = hall_of_fame_iterator_next(&iter)))
		if ((retval = write_hall_of_fame_entry(entry, &ozs.up)))
			break;
	finalize_ozstream(&ozs);
fail_zstream:
	return retval;
}

struct hall_of_fame *load_hall_of_fame(const char *levels_name,
				       const char *config_name)
{
	struct ifstream ifs;
	struct b6_entry *entry;
	struct hall_of_fame *self;
	char name[sizeof(self->name)];
	int len, retval, i;
	len =  snprintf(name, sizeof(name), "%s/%s.%s.hof", get_rw_dir(),
			levels_name, config_name);
	if (len >= sizeof(name))
		return NULL;
	entry = b6_lookup_registry(&hall_of_fame_registry, name);
	if (entry)
		return b6_cast_of(entry, struct hall_of_fame, entry);
	log_i("%s not in cache.", name);
	if (!(self = b6_allocate(&b6_std_allocator, sizeof(*self)))) {
		log_e("out of memory for %s", name);
		return NULL;
	}
	for (i = 0; i < sizeof(name); i += 1)
		if (!(self->name[i] = name[i]))
			break;
	b6_list_initialize(&self->list);
	for (i = 0; i < b6_card_of(self->entries); i += 1)
		b6_list_add_last(&self->list, &self->entries[i].dref);
	if (initialize_ifstream(&ifs, name) ||
	    read_hall_of_fame(self, &ifs.istream)) {
		for (i = 0; i < b6_card_of(self->entries); i += 1) {
			struct hall_of_fame_entry *entry = &self->entries[i];
			entry->name[0] = '\0';
			entry->score = 0UL;
			entry->level = 0UL;
		}
		log_w("created %s", name);
	} else
		log_i("loaded %s", name);
	retval = b6_register(&hall_of_fame_registry, &self->entry, self->name);
	if (retval) {
		b6_deallocate(&b6_std_allocator, self);
		log_e("could not register %s (%d)", name, retval);
		self = NULL;
	}
	return self;
}

int save_hall_of_fame(struct hall_of_fame *self)
{
	struct ofstream ofs;
	int retval;
	if (!(retval = initialize_ofstream(&ofs, self->name))) {
		retval = write_hall_of_fame(self, &ofs.ostream);
		finalize_ofstream(&ofs);
	}
	return retval;
}
