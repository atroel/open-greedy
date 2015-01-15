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

#ifndef LOG_H
#define LOG_H

#include "b6/clock.h"

#define log_(level, format, args...) \
	do { \
		static const char lsym[] = { 'D', 'I', 'W', 'E', 'P' }; \
		int lnum = b6_unlikely(level > LOG_PANIC) ? LOG_PANIC : level; \
		unsigned long long int time = 0; \
		if (b6_likely(lnum < log_level)) \
			break; \
		if (b6_likely(log_clock)) \
			time = b6_get_clock_time(log_clock); \
		log_internal(lnum, time, "%c %llu %s: " format "\n", \
			     lsym[lnum], time, __func__, ##args); \
	} while(0)

#ifdef NDEBUG
#define log_d(format, args...) do {} while(0)
#else
#define log_d(format, args...) log_(LOG_DEBUG, format, ##args)
#endif

#define log_i(format, args...) log_(LOG_INFO, format, ##args)
#define log_w(format, args...) log_(LOG_WARNING, format, ##args)
#define log_e(format, args...) log_(LOG_ERROR, format, ##args)
#define log_p(format, args...) log_(LOG_PANIC, format, ##args)

enum log_level {
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_PANIC,
};

extern enum log_level log_level;
extern const struct b6_clock *log_clock;

static inline void set_log_level(enum log_level level) { log_level = level; }

extern int log_internal(enum log_level, unsigned long long int,
			const char *format, ...);

extern void log_flush(void);

#endif /* LOG_H */
