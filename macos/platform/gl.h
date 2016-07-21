/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014-2016 Arnaud TROEL
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

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#define gl_ext_gen_buffers glGenBuffers
#define gl_ext_delete_buffers glDeleteBuffers
#define gl_ext_buffer_data glBufferData
#define gl_ext_map_buffer glMapBuffer
#define gl_ext_unmap_buffer glUnmapBuffer
#define gl_ext_bind_buffer glBindBuffer

static inline int gl_buffer_extension_is_supported(void) { return 1; }

#endif /* PLATFORM_GL_H */
