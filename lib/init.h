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

#ifndef INIT_H
#define INIT_H

#include <b6/refs.h>
#include <b6/utils.h>

#define register_init(function) \
	b6_ctor(register_init_ ## function); \
	void register_init_ ## function(void) { \
		static struct initializer i; \
		i.func = function; \
		register_initializer(&i); \
	} \
	b6_ctor(register_init_ ## function)

#define register_exit(function) \
	b6_ctor(register_exit_ ## function); \
	void register_exit_ ## function(void) { \
		static struct finalizer f; \
		f.func = function; \
		register_finalizer(&f); \
	} \
	b6_ctor(register_exit_ ## function)

extern void init_all(void);
extern void exit_all(void);

struct initializer {
	struct b6_sref sref;
	int (*func)(void);
};

struct finalizer {
	struct b6_sref sref;
	void (*func)(void);
};

extern void register_initializer(struct initializer*);
extern void register_finalizer(struct finalizer*);

#endif /* INIT_H */
