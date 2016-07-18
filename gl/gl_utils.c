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

#include "gl_utils.h"
#include "core/rgba.h"
#include "lib/log.h"
#include "lib/std.h"
#include <b6/cmdline.h>

static int use_gl_ext = 1;
b6_flag(use_gl_ext, bool);

#define NO_GL_ID ((GLuint)-1)

GLenum gl_error = 0;

static int cache_gl_texture = 1;
b6_flag(cache_gl_texture, bool);

static GLuint current_texture_id = NO_GL_ID;

void bind_gl_texture(GLuint texture)
{
	if (!cache_gl_texture || texture != current_texture_id) {
		current_texture_id = texture;
		glBindTexture(GL_TEXTURE_2D, current_texture_id);
	}
}

void unbind_gl_texture() { current_texture_id = NO_GL_ID; }

void make_gl_texture(GLuint id, const struct rgba *rgba)
{
	gl_call(bind_gl_texture(id));
	gl_call(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
	gl_call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR));
	gl_call(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				GL_LINEAR));
	gl_call(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP));
	gl_call(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP));
        gl_call(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0,
			     GL_RGBA, GL_UNSIGNED_BYTE, rgba->p));
	gl_call(glFlush());
}

static void initialize_gl_buffer(struct gl_buffer *self,
				 const struct gl_buffer_ops *ops)
{
	self->ops = ops;
	b6_array_initialize(&self->t, &b6_std_allocator, 2 * sizeof(GLfloat));
	b6_array_initialize(&self->v, &b6_std_allocator, 2 * sizeof(GLfloat));
}

static void finalize_gl_buffer(struct gl_buffer *self)
{
	b6_array_finalize(&self->t);
	b6_array_finalize(&self->v);
}

static int push_gl_cli_buffer(struct gl_buffer *gl_buffer)
{
	float *t = b6_array_get(&gl_buffer->t, 0);
	float *v = b6_array_get(&gl_buffer->v, 0);
	logf_i("pushing %u vertices", b6_array_length(&gl_buffer->t));
	gl_call(glTexCoordPointer(2, GL_FLOAT, 0, t));
	gl_call(glVertexPointer(2, GL_FLOAT, 0, v));
	return 0;
}

int initialize_gl_cli_buffer(struct gl_cli_buffer *self)
{
	static const struct gl_buffer_ops ops = { .push = push_gl_cli_buffer, };
	initialize_gl_buffer(&self->gl_buffer, &ops);
	return 0;
}

void finalize_gl_cli_buffer(struct gl_cli_buffer *self)
{
	finalize_gl_buffer(&self->gl_buffer);
}

static void bind_gl_buffer(unsigned int id)
{
	b6_static_assert(sizeof(GLuint) == sizeof(unsigned int));
	gl_call(gl_ext_bind_buffer(GL_ARRAY_BUFFER, id));
}

static void* map_gl_buffer(void)
{
	void *ptr = NULL;
	gl_call(ptr = gl_ext_map_buffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
	b6_check(ptr);
	return ptr;
}

static void unmap_gl_buffer(void)
{
	gl_call(gl_ext_unmap_buffer(GL_ARRAY_BUFFER));
}

static void create_gl_buffer(unsigned int *id)
{
	gl_call(gl_ext_gen_buffers(1, id));
}

static void destroy_gl_buffer(unsigned int id)
{
	gl_call(bind_gl_buffer(NO_GL_ID));
	gl_call(gl_ext_delete_buffers(1, &id));
}

static void alloc_gl_buffer(unsigned long int size)
{
	gl_call(gl_ext_buffer_data(GL_ARRAY_BUFFER, size, NULL,
				   GL_STREAM_DRAW));
	/* GL_STATIC_DRAW, GL_STREAM_DRAW or GL_DYNAMIC_DRAW */
}

int push_gl_srv_buffer(struct gl_buffer *gl_buffer)
{
	struct gl_srv_buffer *self =
		b6_cast_of(gl_buffer, struct gl_srv_buffer, gl_buffer);
	float *p, *q;
	static const unsigned long int min_size = 32768;
	unsigned long int v_size = b6_array_memsize(&gl_buffer->v);
	unsigned long int t_size = b6_array_memsize(&gl_buffer->t);
	unsigned long int size = v_size + t_size;
	unsigned long int len = b6_array_length(&gl_buffer->t);
	unsigned long int max = gl_buffer->t.capacity;
	const float *v = b6_array_get(&gl_buffer->v, 0);
	const float *t = b6_array_get(&gl_buffer->t, 0);
	if (size < v_size || size < t_size) {
		log_e(_s("integer overflow"));
		return -2;
	}
	if (size < min_size)
		size = min_size;
	if (size > self->size) {
		logf_i("allocating %u bytes", size);
		self->size = size;
		if (self->id != (GLuint)~0)
			destroy_gl_buffer(self->id);
		create_gl_buffer(&self->id);
		bind_gl_buffer(self->id);
		alloc_gl_buffer(size);
	}
	logf_i("pushing %u/%u vertices", len, max);
	p = map_gl_buffer();
	q = p + max * 2;
	while (len--) { *p++ = *t++; *p++ = *t++; *q++ = *v++; *q++ = *v++; }
	unmap_gl_buffer();
	gl_call(glTexCoordPointer(2, GL_FLOAT, 0, NULL));
	gl_call(glVertexPointer(2, GL_FLOAT, 0, ((float*)NULL) + max * 2));
	return 0;
}

void finalize_gl_srv_buffer(struct gl_srv_buffer *self)
{
	if (self->id)
		destroy_gl_buffer(self->id);
	finalize_gl_buffer(&self->gl_buffer);
}

int initialize_gl_srv_buffer(struct gl_srv_buffer *self)
{
	static const struct gl_buffer_ops ops = { .push = push_gl_srv_buffer, };
	if (!use_gl_ext || !gl_buffer_extension_is_supported())
		return -1;
	initialize_gl_buffer(&self->gl_buffer, &ops);
	self->id = NO_GL_ID;
	self->size = 0;
	return 0;
}
