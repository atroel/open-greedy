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

#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static FILE *stream = NULL;

const struct b6_clock *log_clock = NULL;

#ifdef NDEBUG
enum log_level log_level = LOG_WARNING;
#else
enum log_level log_level = LOG_DEBUG;
#endif

int log_internal(enum log_level level, const char *format, ...)
{
	va_list ap;
	int retval;
	FILE *fp = stream ? stream : stderr;
	va_start(ap, format);
	if (b6_unlikely(level >= LOG_PANIC)) {
		vfprintf(stderr, format, ap);
		if (fp != stderr)
			vfprintf(fp, format, ap);
		abort();
	}
	retval = vfprintf(fp, format, ap);
	va_end(ap);
	return retval;
}

void initialize_log(void)
{
	stream = stderr;
}
