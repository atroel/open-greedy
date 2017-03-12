/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014-2017 Arnaud TROEL
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

#ifndef PLATFORM_GL_H
#define PLATFORM_GL_H

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

typedef void (APIENTRY *gl_ext_gen_buffers_t)(GLsizei, GLuint*);
typedef void (APIENTRY *gl_ext_delete_buffers_t)(GLsizei, const GLuint*);
typedef void (APIENTRY *gl_ext_buffer_data_t)(GLenum, int, const GLvoid*,
					      GLenum);
typedef GLvoid* (APIENTRY *gl_ext_map_buffer_t)(GLenum, GLenum);
typedef GLboolean (APIENTRY *gl_ext_unmap_buffer_t)(GLenum);
typedef void (APIENTRY *gl_ext_bind_buffer_t)(GLenum, GLuint);

extern gl_ext_gen_buffers_t gl_ext_gen_buffers;
extern gl_ext_delete_buffers_t gl_ext_delete_buffers;
extern gl_ext_buffer_data_t gl_ext_buffer_data;
extern gl_ext_map_buffer_t gl_ext_map_buffer;
extern gl_ext_unmap_buffer_t gl_ext_unmap_buffer;
extern gl_ext_bind_buffer_t gl_ext_bind_buffer;

extern int gl_buffer_extension_is_supported(void);

#endif /* PLATFORM_GL_H */
