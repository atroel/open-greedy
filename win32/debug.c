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

#include <signal.h>
#include <stdlib.h>

#include "lib/log.h"

static void crash_pad(int signum)
{
	switch (signum) {
	case SIGINT:  log_e("interruption"); break;
	case SIGTERM: log_e("termination request"); break;
	case SIGABRT: log_e("aborting"); break;
	case SIGFPE:  log_e("floating point exception"); break;
	case SIGILL:  log_e("illegal instruction"); break;
	case SIGSEGV: log_e("segmentation fault"); break;
	default:
		log_e("caught signal #%d", signum);
	}
	exit(EXIT_FAILURE);
}

#define __install_crash_pad(sig) do { \
	log_i("signal %2d (%s)", sig, #sig); \
	signal(sig, crash_pad); \
} while (0)

void install_crash_pad(void)
{
	__install_crash_pad(SIGABRT);
	__install_crash_pad(SIGFPE);
	__install_crash_pad(SIGILL);
	__install_crash_pad(SIGINT);
	__install_crash_pad(SIGSEGV);
	__install_crash_pad(SIGTERM);
}
