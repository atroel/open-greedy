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

#include "hall_of_fame.h"

#include <b6/registry.h>
#include <b6/utf8.h>

#include "env.h"
#include "json.h"
#include "lib/io.h"
#include "lib/log.h"
#include "lib/rng.h"
#include "lib/std.h"

static const struct b6_utf8 label_key = B6_DEFINE_UTF8("label");
static const struct b6_utf8 level_key = B6_DEFINE_UTF8("level");
static const struct b6_utf8 score_key = B6_DEFINE_UTF8("score");

int initialize_hall_of_fame(struct hall_of_fame *self, struct b6_json *json,
			    const char *path)
{
	struct b6_utf8 utf8[2];
	const char *base = get_rw_dir();
	b6_reset_fixed_allocator(&self->path_allocator, self->path_buffer,
				 sizeof(self->path_buffer));
	b6_initialize_utf8_string(&self->path, &self->path_allocator.allocator);
	b6_utf8_from_ascii(&utf8[0], base);
	b6_utf8_from_ascii(&utf8[1], path);
	if (b6_extend_utf8_string(&self->path, &utf8[0]) ||
	    b6_extend_utf8_string(&self->path, &utf8[1])) {
		log_e("path is too long: %s%s", base, path);
		return -2;
	}
	if (!(self->object = b6_json_new_object(json))) {
		log_e("out of memory");
		b6_finalize_utf8_string(&self->path);
		return -1;
	}
	return 0;
}

void finalize_hall_of_fame(struct hall_of_fame *self)
{
	b6_finalize_utf8_string(&self->path);
	b6_json_unref_value(&self->object->up);
}

static enum b6_json_error set_number(struct b6_json_object *object,
				     const struct b6_utf8* key, double value)
{
	struct b6_json_number *number;
	enum b6_json_error error = B6_JSON_ALLOC_ERROR;
	if (!(number = b6_json_new_number(object->json, value))) {
		log_e("b6_json_new_number: ", b6_json_strerror(error));
		b6_json_unref_value(&object->up);
		return error;
	}
	if ((error = b6_json_set_object(object, key, &number->up))) {
		log_e("b6_json_set_object: ", b6_json_strerror(error));
		b6_json_unref_value(&number->up);
		b6_json_unref_value(&object->up);
	}
	return error;
}

static enum b6_json_error get_number(const struct b6_json_object *object,
				     const struct b6_utf8* key, double *value)
{
	struct b6_json_number *number;
	if (!(number = b6_json_get_object_as(object, key, number)))
		return B6_JSON_ERROR;
	*value = b6_json_get_number(number);
	return B6_JSON_OK;
}

static void swap_entries(struct b6_json_array *a,
			 unsigned int i, unsigned int j)
{
	struct b6_json_value *v = b6_json_ref_value(b6_json_get_array(a, i));
	struct b6_json_value *u = b6_json_ref_value(b6_json_get_array(a, j));
	b6_json_set_array(a, i, u);
	b6_json_set_array(a, j, v);
}

static int comp_entries(const struct b6_json_object *lhs,
			const struct b6_json_object *rhs)
{
	double l = 0, r = 0;
	get_number(lhs, &score_key, &l);
	get_number(rhs, &score_key, &r);
	if (l < r)
		return 1;
	if (l > r)
		return -1;
	get_number(lhs, &level_key, &l);
	get_number(rhs, &level_key, &r);
	if (l < r)
		return 1;
	if (l > r)
		return -1;
	return 0;
}

static void sort_entries(struct b6_json_array *array,
			 unsigned int i, unsigned int n)
{
	struct b6_json_object *obj[2];
	if (n > 2) {
		unsigned int k, j = i + read_random_number_generator() * n;
		if (j > i)
			swap_entries(array, i, j);
		obj[0] = b6_json_get_array_as(array, j, object);
		j = i + 1;
		k = i + n - 1;
		for (;;) {
			obj[1] = b6_json_get_array_as(array, k, object);
			if (comp_entries(obj[0], obj[1]) > 0) {
				unsigned int l = j;
				if (j++ >= k)
					break;
				swap_entries(array, l, k);
			} else if (j >= --k)
				break;
		}
		k = j - i;
		sort_entries(array, i, k);
		sort_entries(array, j, n - k);
	} else if (n == 2) {
		obj[0] = b6_json_get_array_as(array, i, object);
		obj[1] = b6_json_get_array_as(array, i + 1, object);
		if (comp_entries(obj[0], obj[1]) > 0)
			swap_entries(array, i, i + 1);
	}
}

int load_hall_of_fame(struct hall_of_fame *self)
{
	unsigned char zbuf[2048];
	struct izstream zs;
	struct ifstream fs;
	struct json_istream js;
	struct b6_json_parser_info info;
	enum b6_json_error error;
	struct b6_json_iterator it, jt;
	const struct b6_json_pair *pair;
	log_i("%s", self->path.utf8.ptr);
	if (initialize_ifstream(&fs, self->path.utf8.ptr)) {
		log_w("cannot open %s", self->path.utf8.ptr);
		return -2;
	}
	if (initialize_izstream(&zs, &fs.istream, zbuf, sizeof(zbuf))) {
		log_e("cannot create izstream");
		finalize_ifstream(&fs);
		return -1;
	}
	setup_json_istream(&js, &zs.up);
	if ((error = b6_json_parse_object(self->object, &js.up, &info)))
		log_e("json: %s", b6_json_strerror(error));
	finalize_izstream(&zs);
	finalize_ifstream(&fs);
	if (error)
		return -1;
	for (b6_json_setup_iterator(&it, self->object);
	     (pair = b6_json_get_iterator(&it));
	     b6_json_advance_iterator(&it)) {
		struct b6_json_object *o;
		struct b6_json_array *a;
		if (!(o = b6_json_value_as_or_null(pair->value, object)))
			continue;
		for (b6_json_setup_iterator(&jt, o);
		     (pair = b6_json_get_iterator(&jt));
		     b6_json_advance_iterator(&jt))
			if ((a = b6_json_value_as_or_null(pair->value, array)))
				sort_entries(a, 0, b6_json_array_len(a));
	}
	return 0;
}

int save_hall_of_fame(const struct hall_of_fame *self)
{
	unsigned char zbuf[2048];
	struct ozstream zs;
	struct ofstream fs;
	struct json_ostream js;
	struct b6_json_default_serializer serializer;
	enum b6_json_error error;
	log_i("%s", self->path.utf8.ptr);
	initialize_ofstream(&fs, self->path.utf8.ptr);
	if (initialize_ozstream(&zs, &fs.ostream, zbuf, sizeof(zbuf))) {
		log_e("cannot create ozstream");
		finalize_ofstream(&fs);
		return -1;
	}
	initialize_json_ostream(&js, &zs.up);
	b6_json_setup_default_serializer(&serializer);
	if ((error = b6_json_serialize_object(self->object, &js.up,
					      &serializer.up)))
		log_e("json: %s", b6_json_strerror(error));
	finalize_json_ostream(&js);
	finalize_ozstream(&zs);
	finalize_ofstream(&fs);
	return -error;
}

int split_hall_of_fame_entry(struct b6_json_object *object,
			     unsigned int *level, unsigned int *score,
			     const struct b6_utf8 **label)
{
	double l, s;
	struct b6_json_string *j;
	if (get_number(object, &level_key, &l) ||
	    get_number(object, &score_key, &s) ||
	    (!(j = b6_json_get_object_as(object, &label_key, string))))
		return -1;
	*level = l;
	*score = s;
	*label = b6_json_get_string(j);
	return 0;
}

int alter_hall_of_fame_entry(struct b6_json_object *object,
			     const struct b6_utf8* label)
{
	struct b6_json_string *s;
	if (!(s = b6_json_new_string(object->json, label)))
		return -1;
	b6_json_set_object(object, &label_key, &s->up);
	return 0;
}

struct b6_json_object *amend_hall_of_fame(struct b6_json_array *array,
					  unsigned long int level,
					  unsigned long int score) {
	static const int max_len = 10;
	struct b6_json_object *o = b6_json_new_object(array->json);
	unsigned int n = b6_json_array_len(array);
	unsigned int i = 0;
	unsigned int k = n;
	enum b6_json_error error;
	if (!o) {
		log_e("b6_json_new_object: out of memory");
		return NULL;
	}
	if (set_number(o, &level_key, level) ||
	    set_number(o, &score_key, score)) {
		b6_json_unref_value(&o->up);
		return NULL;
	}
	while (i != k) {
		unsigned int j = (i + k) / 2;
		int r = comp_entries(o, b6_json_get_array_as(array, j, object));
		if (r > 0)
			i = j + 1;
		else {
			k = j;
			if (!r)
				break;
		}
	}
	if ((error = b6_json_add_array(array, k, &o->up))) {
		log_e("b6_json_add_array: %s", b6_json_strerror(error));
		b6_json_unref_value(&o->up);
		return NULL;
	}
	while (n >= max_len)
		b6_json_del_array(array, n--);
	if (i > n)
		return NULL;
	return o;
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

static int load_legacy_hall_of_fame(struct b6_json_array *self,
				    const char *levels,
				    const struct b6_utf8 *config)
{
	struct b6_utf8_string path;
	struct b6_fixed_allocator fixed_allocator;
	char buffer[512];
	struct b6_utf8 utf8;
	struct ifstream fs;
	struct izstream zs;
	unsigned char zbuf[512];
	const char *base = get_rw_dir();
	int i, retval;
	b6_reset_fixed_allocator(&fixed_allocator, buffer, sizeof(buffer));
	b6_initialize_utf8_string(&path, &fixed_allocator.allocator);
	if (b6_extend_utf8_string(&path, b6_utf8_from_ascii(&utf8, base)) ||
	    b6_extend_utf8_string(&path, b6_utf8_from_ascii(&utf8, levels)) ||
	    b6_append_utf8_string(&path, '.') ||
	    b6_extend_utf8_string(&path, config) ||
	    b6_append_utf8_string(&path, '.') ||
	    b6_append_utf8_string(&path, 'h') ||
	    b6_append_utf8_string(&path, 'o') ||
	    b6_append_utf8_string(&path, 'f')) {
		log_e("path is too long");
		retval = -1;
		goto fail_fstream;
	}
	log_i("%s", path.utf8.ptr);
	if ((retval = initialize_ifstream(&fs, path.utf8.ptr)))
		goto fail_fstream;
	if (initialize_izstream(&zs, &fs.istream, zbuf, sizeof(zbuf)))
		goto fail_zstream;
	for (i = 0; i < 10; i += 1) {
		char name[17];
		unsigned int level, score;
		struct b6_json_object *o;
		retval = read_istream(&zs.up, name, sizeof(name) - 1);
		if (retval != sizeof(name) - 1 ||
		    (retval = read_u32(&zs.up, &level)) ||
		    (retval = read_u32(&zs.up, &score))) {
			log_e("read: i/o error");
			break;
		}
		retval = -1;
		if (!(o = amend_hall_of_fame(self, level, score)))
			break;
		name[sizeof(name) - 1] = '\0';
		alter_hall_of_fame_entry(o, b6_utf8_from_ascii(&utf8, name));
	}
	finalize_izstream(&zs);
fail_zstream:
	finalize_ifstream(&fs);
fail_fstream:
	b6_finalize_utf8_string(&path);
	return retval;
}

struct b6_json_array *open_hall_of_fame(struct hall_of_fame *self,
					const char *levels,
					const struct b6_utf8 *config)
{
	struct b6_utf8 utf8;
	struct b6_json_object *object;
	struct b6_json_array *array;
	enum b6_json_error error;
	b6_utf8_from_ascii(&utf8, levels);
	if (!(object = b6_json_get_object_as(self->object, &utf8, object))) {
		if (!(object = b6_json_new_object(self->object->json))) {
			log_e("cannot create object");
			return NULL;
		}
		error = b6_json_set_object(self->object, &utf8, &object->up);
		if (error) {
			log_e("cannot set object");
			b6_json_unref_value(&object->up);
			return NULL;
		}
	}
	if (!(array = b6_json_get_object_as(object, config, array))) {
		if (!(array = b6_json_new_array(object->json))) {
			log_e("cannot create array");
			b6_json_del_object_at(self->object, &utf8);
			return NULL;
		}
		if ((error = b6_json_set_object(object, config, &array->up))) {
			log_e("cannot set array");
			b6_json_del_object_at(self->object, &utf8);
			b6_json_unref_value(&array->up);
			return NULL;
		}
	}
	if (!b6_json_array_len(array))
		load_legacy_hall_of_fame(array, levels, config);
	b6_json_ref_value(&array->up);
	return array;
}

void close_hall_of_fame(struct b6_json_array *array)
{
	b6_json_unref_value(&array->up);
}
