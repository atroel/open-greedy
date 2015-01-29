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

#include "env.h"
#include <b6/cmdline.h>

static const char *ro_dir = NULL;
b6_flag(ro_dir, string);

static const char *rw_dir = NULL;
b6_flag(rw_dir, string);

static const char *name = NULL;
b6_flag(name, string);

extern const char *get_platform_ro_dir(void);
extern const char *get_platform_rw_dir(void);
extern const char *get_platform_user_name(void);

const char *get_ro_dir(void)
{
	if (!ro_dir)
		ro_dir = get_platform_ro_dir();
	b6_check(ro_dir);
	return ro_dir;
}

const char *get_rw_dir(void)
{
	if (!rw_dir)
		rw_dir = get_platform_rw_dir();
	b6_check(rw_dir);
	return rw_dir;
}

extern const char *get_user_name(void)
{
	if (!name && !(name = get_platform_user_name()))
		name = "player";
	return name;
}
