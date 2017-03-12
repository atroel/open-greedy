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

#ifndef LOG_H
#define LOG_H

#include <b6/clock.h>

#ifdef NDEBUG
#define log_d(...) do {} while(0)
#else
#define log_d(...) log_(LOG_D, __VA_ARGS__)
#endif
#define log_i(...) log_(LOG_I, __VA_ARGS__)
#define log_w(...) log_(LOG_W, __VA_ARGS__)
#define log_e(...) log_(LOG_E, __VA_ARGS__)
#define log_p(...) log_(LOG_P, __VA_ARGS__)

#ifdef NDEBUG
#define logf_d(f, ...) do {} while(0)
#else
#define logf_d(f, ...) logf_(LOG_D, f, __VA_ARGS__)
#endif
#define logf_i(f, ...) logf_(LOG_I, f, __VA_ARGS__)
#define logf_w(f, ...) logf_(LOG_W, f, __VA_ARGS__)
#define logf_e(f, ...) logf_(LOG_E, f, __VA_ARGS__)
#define logf_p(f, ...) logf_(LOG_P, f, __VA_ARGS__)

extern enum log_level log_level;
extern struct b6_clock *log_clock;

enum log_level { LOG_D, LOG_I, LOG_W, LOG_E, LOG_P, };

static inline void set_log_level(enum log_level level) { log_level = level; }

static inline void set_log_clock(struct b6_clock *clock) { log_clock = clock; }

extern void log_flush(void);

#define logf_(level, _format, ...) \
	do { \
		enum log_level _level = level; \
		unsigned long long int _now = 0; \
		if (b6_likely(_level < log_level)) \
			break; \
		if (log_clock) \
			_now = b6_get_clock_time(log_clock); \
		do_log_prolog(_level, _now, __func__); \
		do_log_fmt(_format, __VA_ARGS__); \
		do_log_epilog(_level, _now); \
	} while (0)

#define log_(level, ...) \
	do { \
		void (*const _f)(const char*, ...) = do_log_fmt; \
		void (*const _s)(const char*) = do_log_str; \
		void (*const _t)(const struct b6_utf8*) = do_log_utf8; \
		unsigned long long int _now = 0; \
		enum log_level _level = level; \
		if (b6_likely(_level < log_level)) \
			break; \
		if (log_clock) \
			_now = b6_get_clock_time(log_clock); \
		(void)_f; \
		(void)_s; \
		(void)_t; \
		for (do_log_prolog(_level, _now, __func__), __VA_ARGS__, \
		     do_log_epilog(_level, _now);0;); \
	} while(0)

void do_log_fmt(const char*, ...);
void do_log_str(const char*);
void do_log_utf8(const struct b6_utf8*);
void do_log_prolog(enum log_level, unsigned long long int, const char*);
void do_log_epilog(enum log_level, unsigned long long int);

#endif /* LOG_H */
