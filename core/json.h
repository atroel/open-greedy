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

#ifndef JSON_H
#define JSON_H

#include <b6/json.h>

#include "lib/io.h"
#include "lib/log.h"

extern struct b6_json *get_json(void);

extern void put_json(struct b6_json*);

struct json_istream {
	struct b6_json_istream up;
	struct istream *istream;
};

extern void setup_json_istream(struct json_istream *self,
			       struct istream *istream);

struct json_ostream {
	struct b6_json_ostream up;
	struct ostream *ostream;
};

extern void initialize_json_ostream(struct json_ostream *self,
				    struct ostream *ostream);

extern void finalize_json_ostream(struct json_ostream *self);

extern struct b6_json_object *walk_json(struct b6_json_object *self,
					const void **path_utf8,
					unsigned int *path_size,
					unsigned long int npaths,
					int create);

#endif /* JSON_H */
