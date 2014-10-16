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
#include <CoreServices/CoreServices.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

const char *get_platform_user_name(void)
{
	return getlogin();
}

const char *get_platform_ro_dir(void)
{
	return "../Resources/data";
}

const char *get_platform_rw_dir(void)
{
	static char path[PATH_MAX];
	char documents_path[PATH_MAX];
	FSRef ref;
	int len, error;
	if ((error = FSFindFolder(kUserDomain, kDocumentsFolderType,
				  kCreateFolder, &ref))) {
		log_e("error %d when searching documents directory", error);
		return NULL;
	}
	if ((error = FSRefMakePath(&ref, (UInt8*)&documents_path,
				   sizeof(documents_path)))) {
		log_e("error %d when translating documents directory", error);
		return NULL;
	}
	len = snprintf(path, sizeof(path), "%s/OpenGreedy", documents_path);
	if (len >= sizeof(path)) {
		log_e("path is too long");
		return NULL;
	}
	if (mkdir(path, 0755) && EEXIST != (error = errno)) {
		log_e("error %d when making directory %s", error, path);
		return NULL;
	}
	return path;
}
