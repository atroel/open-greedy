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

#include "lib/log.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

static PFNGLGENBUFFERSARBPROC glGenBuffers = NULL;
static PFNGLDELETEBUFFERSARBPROC glDeleteBuffers = NULL;
static PFNGLBUFFERDATAARBPROC glBufferData = NULL;
static PFNGLMAPBUFFERARBPROC glMapBuffer = NULL;
static PFNGLUNMAPBUFFERARBPROC glUnmapBuffer = NULL;
static PFNGLBINDBUFFERARBPROC glBindBuffer = NULL;

static void *get_gl_extension(const char *name)
{
	void *proc = (void*)glXGetProcAddress((GLubyte*)name);
	if (!proc)
		log_w("GL extension \"%s\" is not available.", name);
	return proc;
}

int supports_gl_buffer(void)
{
	if (!(glGenBuffers = get_gl_extension("glGenBuffers")))
		return -1;
	if (!(glDeleteBuffers = get_gl_extension("glDeleteBuffers")))
		return -1;
	if (!(glBindBuffer = get_gl_extension("glBindBuffer")))
		return -1;
	if (!(glBufferData = get_gl_extension("glBufferData")))
		return -1;
	if (!(glMapBuffer = get_gl_extension("glMapBuffer")))
		return -1;
	if (!(glUnmapBuffer = get_gl_extension("glUnmapBuffer")))
		return -1;
	return 0;

}

void bind_gl_buffer(unsigned int id)
{
	glBindBuffer(GL_ARRAY_BUFFER, id);
}

void* map_gl_buffer(void)
{
	return glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
}

void unmap_gl_buffer(void)
{
	glUnmapBuffer(GL_ARRAY_BUFFER);
}

void create_gl_buffer(unsigned int *id)
{
	glGenBuffers(1, id);
}

void destroy_gl_buffer(unsigned int id)
{
	glDeleteBuffers(1, &id);
}

void alloc_gl_buffer(unsigned long int size)
{
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_DRAW);
	/* GL_STATIC_DRAW, GL_STREAM_DRAW or GL_DYNAMIC_DRAW */
}
