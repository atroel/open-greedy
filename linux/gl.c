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

#include "lib/log.h"
#include "platform/gl.h"

#include <GL/glx.h>

gl_ext_gen_buffers_t gl_ext_gen_buffers = NULL;
gl_ext_delete_buffers_t gl_ext_delete_buffers = NULL;
gl_ext_buffer_data_t gl_ext_buffer_data = NULL;
gl_ext_map_buffer_t gl_ext_map_buffer = NULL;
gl_ext_unmap_buffer_t gl_ext_unmap_buffer = NULL;
gl_ext_bind_buffer_t gl_ext_bind_buffer = NULL;

static void *get_gl_extension(const char *name)
{
	void *proc = (void*)glXGetProcAddress((GLubyte*)name);
	if (!proc)
		logf_w("\"%s\" is not supported", name);
	return proc;
}

int gl_buffer_extension_is_supported(void)
{
	static short int initialized = 0;
	static short int supported = 0;
	if (initialized)
		goto done;
	initialized = 1;
	if (!(gl_ext_gen_buffers = get_gl_extension("glGenBuffers")))
		goto done;
	if (!(gl_ext_delete_buffers = get_gl_extension("glDeleteBuffers")))
		goto done;
	if (!(gl_ext_bind_buffer = get_gl_extension("glBindBuffer")))
		goto done;
	if (!(gl_ext_buffer_data = get_gl_extension("glBufferData")))
		goto done;
	if (!(gl_ext_map_buffer = get_gl_extension("glMapBuffer")))
		goto done;
	if (!(gl_ext_unmap_buffer = get_gl_extension("glUnmapBuffer")))
		goto done;
	supported = 1;
done:
	return supported;
}
