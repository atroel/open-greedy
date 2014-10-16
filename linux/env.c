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

#include "lib/log.h"
#include <b6/cmdline.h>
#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char *get_platform_user_name(void)
{
	return getlogin();
}

const char *get_platform_ro_dir(void)
{
	return "data";
}

const char *get_platform_rw_dir(void)
{
	static char path[1024];
	struct passwd *pw;
	int len, error;
	pw = getpwuid(getuid());
	if (!pw || !pw->pw_dir) {
		log_e("cannot get user's home directory");
		return NULL;
	}
	len = snprintf(path, sizeof(path), "%s/.opengreedy", pw->pw_dir);
	if (len >= sizeof(path)) {
		log_e("path is too long");
		return NULL;
	}
	if (mkdir(path, 0755) && EEXIST != (error = errno)) {
		log_e("error %d when making directory: %s", error, path);
		return NULL;
	}
	return path;
}
