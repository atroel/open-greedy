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

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

const char *get_platform_user_name(void)
{
	return getlogin();
}

static int probe_ro_dir(const char *ro_dir)
{
	DIR *dir = opendir(ro_dir);
	if (dir)
		closedir(dir);
	return !!dir;
}

#ifndef RO_DIR
#define RO_DIR ""
#endif

const char *get_platform_ro_dir(void)
{
	static char chunk[512];
	static const char *ro_dir = RO_DIR;
	const char *src = ro_dir;
	while (*src) {
		char *dst = chunk;
		int skip = 0;
		do {
			*dst = *src;
			if (!*dst)
				break;
			if (*src++ == ',') {
				*dst = '\0';
				break;
			}
			dst += 1;
		} while (!(skip = dst == chunk + sizeof(chunk)));
		if (skip) {
			while (*src)
			       if (*src++ == ',')
				       break;
			continue;
		}
		if (probe_ro_dir(chunk))
			return chunk;
	}
	return NULL;
}
