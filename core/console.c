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

#include "console.h"

#include <b6/flags.h>

static unsigned short int ws = 0;
b6_flag(ws, ushort);

static unsigned short int hs = 0;
b6_flag(hs, ushort);

#ifdef NDEBUG
static int fs = 1;
#else
static int fs = 0;
#endif
b6_flag(fs, bool);

static int vs = 0;
b6_flag(vs, bool);

B6_REGISTRY_DEFINE(__console_registry);

unsigned short int get_console_width() { return ws; }
unsigned short int get_console_height() { return hs; }
int get_console_fullscreen() { return fs; }
int get_console_vsync() { return vs; }
