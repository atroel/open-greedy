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

#include <windows.h>
#include <b6/clock.h>
#include "lib/log.h"

struct win32_clock {
	struct b6_clock up;
	double uperiod; /* usec/ticks */
	unsigned long long int base; /* base ticks */
};

static unsigned long long int get_win32_ticks(void) {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

static unsigned long long int get_time_win32(const struct b6_clock *up)
{
	struct win32_clock *self = b6_cast_of(up, struct win32_clock, up);
	unsigned long long int ticks = get_win32_ticks();
	return (unsigned long long int)((ticks - self->base) * self->uperiod);
}

static void wait_win32(const struct b6_clock *up,
		       unsigned long long int delay_us)
{
	unsigned long long int delta, after, before = get_time_win32(up);
	for (;;) {
		Sleep(delay_us / 1000);
		after = get_time_win32(up);
		delta = after - before;
		if (delta >= delay_us)
			break;
		delay_us -= delta;
		before = after;
	}
}

b6_ctor(register_win32_clock);
static void register_win32_clock(void)
{
	static const struct b6_clock_ops ops = {
		.get_time = get_time_win32,
		.wait = wait_win32,
	};
	static struct win32_clock win32_clock = {
		.up = { .ops = &ops, },
	};
	static struct b6_named_clock win32_named_clock = {
		.clock = &win32_clock.up,
	};
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li)) {
		log_e(_s("QueryPerformanceFrequency failed"));
		return;
	}
	win32_clock.uperiod = 1e6/(double)li.QuadPart;
	win32_clock.base = get_win32_ticks();
	if (b6_register_named_clock(&win32_named_clock, B6_UTF8("win32")))
		log_e(_s("could not register win32 clock"));
}
