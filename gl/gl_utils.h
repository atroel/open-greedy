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

#ifndef GL_UTILS_H
#define GL_UTILS_H

#include "platform/gl.h"

#include "lib/log.h"

#include <b6/array.h>
#include <b6/utils.h>

struct rgba;

extern GLenum gl_error;

#define gl_call(_exp) do {\
	GLenum error; \
	_exp; \
	error = glGetError(); \
	if (b6_unlikely(error && error != gl_error)) \
		log_e("OpenGL error 0x%04x %s", error, #_exp); \
	gl_error = error; \
} while (0)

extern void bind_gl_texture(GLuint texture);

extern void unbind_gl_texture(void);

extern void make_gl_texture(GLuint id, const struct rgba *rgba);

struct gl_buffer {
	const struct gl_buffer_ops *ops;
	struct b6_array t;
	struct b6_array v;
};

struct gl_buffer_ops {
	int (*push)(struct gl_buffer *self);
};

static inline void clear_gl_buffer(struct gl_buffer *self)
{
	b6_array_clear(&self->t);
	b6_array_clear(&self->v);
}

static inline int append_gl_buffer(struct gl_buffer *self,
				   float u1, float v1, float u2, float v2,
				   float x1, float y1, float x2, float y2)
{
	float *t = b6_array_extend(&self->t, 6);
	float *v = b6_array_extend(&self->v, 6);
	if (t && v) {
		*t++ = u2; *t++ = v2; *v++ = x2; *v++ = y2;
		*t++ = u1; *t++ = v2; *v++ = x1; *v++ = y2;
		*t++ = u1; *t++ = v1; *v++ = x1; *v++ = y1;
		*t++ = u1; *t++ = v1; *v++ = x1; *v++ = y1;
		*t++ = u2; *t++ = v1; *v++ = x2; *v++ = y1;
		*t++ = u2; *t++ = v2; *v++ = x2; *v++ = y2;
		return 0;
	}
	if (t)
		b6_array_reduce(&self->t, 6);
	if (v)
		b6_array_reduce(&self->v, 6);
	return -1;
}

static inline int push_gl_buffer(struct gl_buffer *self)
{
	return self->ops->push(self);
}

struct gl_cli_buffer {
	struct gl_buffer gl_buffer;
};

extern int initialize_gl_cli_buffer(struct gl_cli_buffer*);

extern void finalize_gl_cli_buffer(struct gl_cli_buffer*);

struct gl_srv_buffer {
	struct gl_buffer gl_buffer;
	GLuint id;
	unsigned long int size; /* size in bytes of the remote buffer */
};

extern int initialize_gl_srv_buffer(struct gl_srv_buffer*);

extern void finalize_gl_srv_buffer(struct gl_srv_buffer*);

#endif  /* GL_UTILS_H */
