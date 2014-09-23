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

#ifdef NDEBUG
#define log_d(format, args...) do {} while(0)
#else
#define log_d(format, args...) \
	do { \
		if (b6_likely(LOG_DEBUG < log_level)) \
			break; \
		log_internal(LOG_DEBUG, "D %llu %s: " format "\n", \
			     log_clock ? b6_get_clock_time(log_clock) : 0ULL, \
			     __func__, ##args); \
	} while(0)
#endif

#define log_i(format, args...) \
	do { \
		if (b6_likely(LOG_INFO < log_level)) \
			break; \
		log_internal(LOG_INFO, "I %llu %s: " format "\n", \
			     log_clock ? b6_get_clock_time(log_clock) : 0ULL, \
			     __func__, ##args); \
	} while(0)

#define log_w(format, args...) \
	do { \
		if (b6_likely(LOG_WARNING < log_level)) \
			break; \
		log_internal(LOG_WARNING, "W %llu %s: " format "\n", \
			     log_clock ? b6_get_clock_time(log_clock) : 0ULL, \
			     __func__, ##args); \
	} while(0)

#define log_e(format, args...) \
	do { \
		if (b6_likely(LOG_ERROR < log_level)) \
			break; \
		log_internal(LOG_ERROR, "E %llu %s: " format "\n", \
			     log_clock ? b6_get_clock_time(log_clock) : 0ULL, \
			     __func__, ##args); \
	} while(0)

#define log_p(format, args...) \
	log_internal(LOG_PANIC, "P %llu %s: " format "\n", \
		     log_clock ? b6_get_clock_time(log_clock) : 0ULL, \
		     __func__, ##args)

enum log_level {
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_PANIC,
};

extern enum log_level log_level;
extern const struct b6_clock *log_clock;

extern void initialize_log(void);

static inline void set_log_level(enum log_level level) { log_level = level; }

extern int log_internal(enum log_level, const char *format, ...);

#endif /* LOG_H */
