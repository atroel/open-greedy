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

#include <b6/clock.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef NDEBUG
enum log_level log_level = LOG_W;
#else
enum log_level log_level = LOG_D;
#endif

struct b6_clock *log_clock = NULL;

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

static void *log_map(struct log_buffer *self, unsigned int *len)
{
	if (self->off >= self->len)
		return NULL;
	*len = self->len - self->off;
	return (char*)self->buf + self->off;
}

static void log_unmap(struct log_buffer *self, void *ptr)
{
	self->off = (char*)ptr - (char*)self->buf;
	if (self->off > self->len)
		b6_check(self->off <= self->len);
}

static char buf[4096];

static struct log_buffer log_buffer = {
	.off = 0,
	.len = sizeof(buf),
	.buf = buf,
};

static unsigned long long int last_log_flush_time = 0;

static void log_flush_at(unsigned long long int now)
{
	last_log_flush_time = now;
	flush_log_buffer(&log_buffer);
}

static void maybe_log_flush_at(unsigned long long int now)
{
	if (b6_unlikely(now > last_log_flush_time &&
			now - last_log_flush_time > 2 * 1000 * 1000))
		log_flush_at(now);
}

void log_flush(void) {
	log_flush_at(log_clock ? b6_get_clock_time(log_clock) : 0ULL);
}

void do_log_fmt(const char *format, ...)
{
	int retval;
	va_list ap;
	unsigned int len;
	char *ptr = log_map(&log_buffer, &len);
	if (ptr) {
		va_start(ap, format);
		retval = vsnprintf(ptr, len, format, ap);
		va_end(ap);
		if (retval < len)
			goto done;
	}
	log_flush();
	if (!(ptr = log_map(&log_buffer, &len)))
		return;
	va_start(ap, format);
	retval = vsnprintf(ptr, len, format, ap);
	va_end(ap);
done:
	log_unmap(&log_buffer, ptr + retval);
}

void do_log_str(const char *s)
{
	unsigned int len;
	char *ptr = log_map(&log_buffer, &len);
	if (!ptr) {
		log_flush();
		if (!(ptr = log_map(&log_buffer, &len)))
			return;
	}
	while ((*ptr++ = *s++))
		if (!--len) {
			log_unmap(&log_buffer, ptr);
			log_flush();
			if (!(ptr = log_map(&log_buffer, &len)))
				return;
		}
	log_unmap(&log_buffer, ptr);
}

void do_log_utf8(const struct b6_utf8 *utf8)
{
	unsigned int dlen;
	char *dptr = log_map(&log_buffer, &dlen);
	const char *sptr = utf8->ptr;
	unsigned int slen = utf8->nbytes;
	if (!dptr) {
		log_flush();
		if (!(dptr = log_map(&log_buffer, &dlen)))
			return;
	}
	while (slen--) {
		*dptr++ = *sptr++;
		if (!--dlen) {
			log_unmap(&log_buffer, dptr);
			if (!(dptr = log_map(&log_buffer, &dlen)))
				return;
		}
	}
	log_unmap(&log_buffer, dptr);
}

void do_log_prolog(enum log_level level, unsigned long long int now,
		   const char *func)
{
	static char sym[] = { 'D', 'I', 'W', 'E', 'P' };
	char buf[64];
	char *ptr = buf + sizeof(buf);
	*--ptr = '\0';
	*--ptr = ' ';
	do {
		*--ptr = '0' + now % 10;
		now /= 10;
	} while (now > 0);
	*--ptr = ' ';
	*--ptr = sym[level];
	do_log_str(ptr);
	do_log_str(func);
	do_log_str(": ");
}

void do_log_epilog(enum log_level level, unsigned long long int now)
{
	do_log_str("\n");
	if (b6_unlikely(level >= LOG_P)) {
		flush_log_buffer(&log_buffer);
		abort();
	}
	if (level == LOG_E)
		log_flush_at(now);
	else
		maybe_log_flush_at(now);
}
