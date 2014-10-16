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

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>

#include "lib/log.h"

void log_backtrace(void)
{
	void *ptr[128];
	int len;
	char **buf;
	len = backtrace(ptr, b6_card_of(ptr));
	buf = backtrace_symbols(ptr, len);
	if (buf) {
		int i;
		for (i = 0; i < len; i += 1)
			log_e("%s", buf[i]);
		free(buf);
		if (len == b6_card_of(ptr))
			log_e("backtrace is truncated");
	} else
		log_e("failed getting backtrace symbols");
}

static void crash_pad(int signum)
{
	switch (signum) {
	case SIGINT:  log_e("interruption"); break;
	case SIGTERM: log_e("termination request"); break;
	case SIGABRT: log_e("aborting"); log_backtrace(); break;
	case SIGFPE:  log_e("floating point exception"); log_backtrace(); break;
	case SIGILL:  log_e("illegal instruction"); log_backtrace(); break;
	case SIGSEGV: log_e("segmentation fault"); log_backtrace(); break;
	case SIGBUS:  log_e("bus error"); log_backtrace(); break;
	default:
		log_e("caught signal #%d", signum);
		log_backtrace();
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
	__install_crash_pad(SIGBUS);
}
