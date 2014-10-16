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

#include "data.h"
#include "lib/log.h"
#include <b6/cmdline.h>
#include <stdarg.h>
#include <stdio.h>

static const char *skin = "greedy";
b6_flag(skin, string);

static const char *fallback = "default";
b6_flag_named(fallback, string, "skin_fallback");

const char *audio_data_type = "audio";
const char *image_data_type = "image";
const char *level_data_type = "level";

B6_REGISTRY_DEFINE(__data_registry);

const char *get_skin_id(void)
{
	return skin;
}

int make_data_path(char *buf, unsigned long int len, const char *root,
		   const char *type, const char *what)
{
	return -(snprintf(buf, len, "%s.%s.%s", root, type, what) >= len);
}

int get_image_data_no_fallback(const char *skin_id, const char *data_id,
			       void *param, struct data_entry **entry,
			       struct image_data **data)
{
	if (!(*entry = lookup_data(skin_id, image_data_type, data_id))) {
		log_w("cannot find %s.%s.%s", skin_id, image_data_type,
		      data_id);
		return -1;
	}
	struct istream *istream;
	int retval;
	if (!(istream = get_data(*entry))) {
		log_w("cannot get %s.%s.%s", skin_id, image_data_type, data_id);
		return -1;
	}
	*data = b6_cast_of(istream, struct image_data, up);
	if (!(*data)->ops->ctor || !(retval = (*data)->ops->ctor(*data, param)))
		return 0;
	put_data(*entry, istream);
	log_w("cannot read %s.%s.%s", skin_id, image_data_type, data_id);
	return -1;
}

int get_image_data(const char *skin_id, const char *data_id, void *param,
		   struct data_entry **entry, struct image_data **data)
{
	int retval;
	retval = get_image_data_no_fallback(skin_id, data_id, param,
					    entry, data);
	if (retval && *fallback) {
		log_w("falling back to %s for %s", fallback, data_id);
		retval = get_image_data_no_fallback(fallback, data_id, param,
						    entry, data);
	}
	return retval;
}

void put_image_data(struct data_entry *entry, struct image_data *data)
{
	if (data->ops->dtor)
		data->ops->dtor(data);
	put_data(entry, &data->up);
}
