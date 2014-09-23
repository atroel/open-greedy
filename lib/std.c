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

#include "std.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *std_allocate(struct b6_allocator *self, unsigned long int size)
{
	return malloc(size);
}

static void *std_reallocate(struct b6_allocator *self, void *ptr,
				unsigned long int size)
{
	return realloc(ptr, size);
}

static void std_deallocate(struct b6_allocator *self, void *ptr)
{
	free(ptr);
}

static const struct b6_allocator_ops std_allocator_ops = {
	.allocate = std_allocate,
	.reallocate = std_reallocate,
	.deallocate = std_deallocate,
};

struct b6_allocator b6_std_allocator = { .ops = &std_allocator_ops };

void b6_assert_handler(const char *func, const char *file, int line, int type,
		       const char *cond)
{
	fprintf(stderr, "%s:%d: assertion failure (%s)\n", file, line, cond);
	exit(EXIT_FAILURE);
}
