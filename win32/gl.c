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
#include <windows.h>

#define GL_ARRAY_BUFFER                   0x8892
#define GL_READ_ONLY                      0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_READ_WRITE                     0x88BA
#define GL_BUFFER_ACCESS                  0x88BB
#define GL_BUFFER_MAPPED                  0x88BC
#define GL_BUFFER_MAP_POINTER             0x88BD
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA

static void (APIENTRY *glBindBuffer)(GLenum, GLuint) = NULL;
static void (APIENTRY *glDeleteBuffers)(GLsizei, const GLuint*) = NULL;
static void (APIENTRY *glGenBuffers)(GLsizei, GLuint*) = NULL;
static void (APIENTRY *glBufferData)(GLenum, int, const GLvoid*, GLenum) = NULL;
static GLvoid* (APIENTRY *glMapBuffer)(GLenum, GLenum) = NULL;
static GLboolean (APIENTRY *glUnmapBuffer)(GLenum) = NULL;

static void *get_gl_extension(const char *name)
{
	void *proc = (void*)wglGetProcAddress(name);
	if (!proc)
		log_w("GL extension \"%s\" is not available.", name);
	return proc;
}

int supports_gl_buffer(void)
{
	if (!(glGenBuffers = get_gl_extension("glGenBuffers")))
		return 0;
	if (!(glDeleteBuffers = get_gl_extension("glDeleteBuffers")))
		return 0;
	if (!(glBindBuffer = get_gl_extension("glBindBuffer")))
		return 0;
	if (!(glBufferData = get_gl_extension("glBufferData")))
		return 0;
	if (!(glMapBuffer = get_gl_extension("glMapBuffer")))
		return 0;
	if (!(glUnmapBuffer = get_gl_extension("glUnmapBuffer")))
		return 0;
	return 1;

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

void alloc_gl_buffer(unsigned int id, unsigned long int size)
{
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_DRAW);
	/* GL_STATIC_DRAW, GL_STREAM_DRAW or GL_DYNAMIC_DRAW */
}
