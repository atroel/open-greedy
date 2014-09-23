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

#include "init.h"

#include <b6/deque.h>

B6_DEQUE_DEFINE(init_deque);
B6_DEQUE_DEFINE(exit_deque);

void init_all(void)
{
	struct b6_sref *sref;
	for (sref = b6_deque_first(&init_deque);
	     sref != b6_deque_tail(&init_deque);
	     sref = b6_deque_walk(&init_deque, sref, B6_NEXT)) {
		struct initializer *self =
			b6_cast_of(sref, struct initializer, sref);
		self->func();
	}
}

void register_initializer(struct initializer *self)
{
	b6_deque_add_last(&init_deque, &self->sref);
}

void exit_all(void)
{
	struct b6_sref *sref;
	for (sref = b6_deque_first(&exit_deque);
	     sref != b6_deque_tail(&exit_deque);
	     sref = b6_deque_walk(&exit_deque, sref, B6_NEXT)) {
		struct initializer *self =
			b6_cast_of(sref, struct initializer, sref);
		self->func();
	}
}

void register_finalizer(struct finalizer *self)
{
	b6_deque_add_first(&exit_deque, &self->sref);
}
