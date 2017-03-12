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

#include <time.h>
#include <unistd.h>
#include <b6/utf8.h>
#include <b6/utils.h>
#include <b6/clock.h>

static unsigned long long int linux_get_time(const struct b6_clock *clock)
{
	struct timespec timespec;
	int retval = clock_gettime(CLOCK_MONOTONIC_RAW, &timespec);
	unsigned long long int us = timespec.tv_sec;
	b6_check(!retval);
	us *= 1000 * 1000;
	us += timespec.tv_nsec / 1000;
	return us;
}

static void linux_wait(const struct b6_clock *clock,
		       unsigned long long int delay_us)
{
	usleep(delay_us);
}

b6_ctor(register_linux_clock);
void register_linux_clock(void)
{
	static const struct b6_clock_ops linux_clock_ops = {
		.get_time = linux_get_time,
		.wait = linux_wait,
	};
	static struct b6_clock linux_clock = { .ops = &linux_clock_ops, };
	static struct b6_named_clock linux_named_clock = {
		.clock = &linux_clock,
	};
	b6_register_named_clock(&linux_named_clock, B6_UTF8("linux"));
}
