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
#include <windows.h>

static char *unicode_to_ascii(const wchar_t *buf, DWORD len, char *p, int n)
{
	while (len-- && n > 0) {
		wchar_t unicode = *buf++;
		if (!unicode) {
			n -= 1;
			*p++ = '\0';
			break;
		}
		if (unicode < 32)
			continue;
		if (unicode < 128) {
			n -= 1;
			*p++ = (char)unicode;
			continue;
		}
		switch (unicode) {
		case 192: case 193: case 194: case 195: case 196: case 197:
			n -= 1;
			*p++ = 'A';
			break;
		case 198:
			if (n > 1) {
				*p++ = 'A';
				*p++ = 'E';
				n -= 2;
			} else
				n = 0;
			break;
		case 199:
			n -= 1;
			*p++ = 'C';
			break;
		case 200: case 201: case 202: case 203:
			n -= 1;
			*p++ = 'E';
			break;
		case 204: case 205: case 206: case 207:
			n -= 1;
			*p++ = 'I';
			break;
		case 208:
			n -= 1;
			*p++ = 'D';
			break;
		case 209:
			n -= 1;
			*p++ = 'N';
			break;
		case 210: case 211: case 212: case 213: case 214: case 216:
			n -= 1;
			*p++ = 'O';
			break;
		case 217: case 218: case 219: case 220:
			n -= 1;
			*p++ = 'U';
			break;
		case 221:
			n -= 1;
			*p++ = 'Y';
			break;
		case 224: case 225: case 226: case 227: case 228: case 229:
			n -= 1;
			*p++ = 'a';
			break;
		case 230:
			if (n > 1) {
				*p++ = 'a';
				*p++ = 'e';
				n -= 2;
			} else
				n = 0;
			break;
		case 231:
			n -= 1;
			*p++ = 'c';
			break;
		case 232: case 233: case 234: case 235:
			n -= 1;
			*p++ = 'e';
			break;
		case 236: case 237: case 238: case 239:
			n -= 1;
			*p++ = 'i';
			break;
		case 240:
			n -= 1;
			*p++ = 'd';
			break;
		case 241:
			n -= 1;
			*p++ = 'n';
			break;
		case 242: case 243: case 244: case 245: case 246: case 247:
			n -= 1;
			*p++ = 'o';
			break;
		case 248: case 249: case 250: case 251: case 252:
			n -= 1;
			*p++ = 'u';
			break;
		case 253:
			n -= 1;
			*p++ = 'y';
			break;
		case 338:
			if (n > 1) {
				*p++ = 'O';
				*p++ = 'E';
				n -= 2;
			} else
				n = 0;
			break;
		case 339:
			if (n > 1) {
				*p++ = 'o';
				*p++ = 'e';
				n -= 2;
			} else
				n = 0;
			break;
		default:
			break;
		}
	}
	return p;
}

const char *get_platform_user_name(void)
{
	static char ascii[256];
	wchar_t buf[256];
	DWORD len = sizeof(buf);
	if (!GetUserNameW(buf, &len)) {
		log_e("could not get user name: %d", GetLastError());
		return NULL;
	}
	*unicode_to_ascii(buf, len, ascii, sizeof(ascii) - 1) = '\0';
	return ascii;
}

const char *get_platform_ro_dir(void)
{
	return "data";
}
