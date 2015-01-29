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

#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

const struct b6_clock *log_clock = NULL;

#ifdef NDEBUG
enum log_level log_level = LOG_WARNING;
#else
enum log_level log_level = LOG_DEBUG;
#endif

static FILE *stream = NULL;

static FILE *get_log_stream(void) { return stream ? stream : stderr; }

struct log_buffer {
	unsigned short int off;
	unsigned short int len;
	void *buf;
};

static int flush_log_buffer(struct log_buffer *self)
{
	FILE *fp = get_log_stream();
	size_t len = self->off;
	unsigned char *ptr = self->buf;
	int retval = 0;
	while (len) {
		size_t wlen = fwrite(ptr, 1, len, fp);
		if (!wlen) {
			retval = -1;
			break;
		}
		len -= wlen;
		ptr += wlen;
	}
	self->off = 0;
	return retval;
}

static int write_log_buffer(struct log_buffer *self, const char *format,
			    va_list ap)
{
	size_t len = self->len - self->off;
	char *ptr = (char*)self->buf + self->off;
	int retval = vsnprintf(ptr, len, format, ap);
	if (retval < 0)
		return -2;
	if (retval >= len)
		return -1;
	self->off += retval;
	return 0;
}

static char buf[4096];

static struct log_buffer log_buffer = {
	.off = 0,
	.len = sizeof(buf),
	.buf = buf,
};

void log_flush(void) { flush_log_buffer(&log_buffer); }

static unsigned long long int last_log_flush_time = 0;

int log_internal(enum log_level level, unsigned long long int time,
		 const char *format, ...)
{
	int retval;
	va_list ap;
	if (b6_unlikely(level >= LOG_PANIC)) {
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
		if (get_log_stream() != stderr)
			write_log_buffer(&log_buffer, format, ap);
		log_flush();
		abort();
	}
	va_start(ap, format);
	retval = write_log_buffer(&log_buffer, format, ap);
	va_end(ap);
	if (retval == -1) {
		log_flush();
		va_start(ap, format);
		retval = write_log_buffer(&log_buffer, format, ap);
		va_end(ap);
	}
	if (b6_unlikely(time > last_log_flush_time &&
			time - last_log_flush_time > 2 * 1000 * 1000)) {
		log_flush();
		last_log_flush_time = time;
	}
	return retval;
}
