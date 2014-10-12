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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <b6/cmdline.h>
#include "lib/io.h"

static void write_head(struct ostream *ostream)
{
	static const char a[] =
		"#include \"lib/embedded.h\"\n"
		"static const unsigned char data[] = {\n";
	write_ostream(ostream, a, sizeof(a) - 1);
}

static ssize_t write_body(struct istream *is, struct ostream *os,
			  void *buf, unsigned long long int len)
{
	ssize_t rlen, wlen, size;
	for (size = 0; ; size += rlen) {
		rlen = read_istream(is, buf, len);
		if (!rlen)
			break;
		if (rlen < 0)
			return -1;
		wlen = write_ostream(os, buf, rlen);
		if (rlen > wlen)
			return -2;
	}
	return size;
}

static void write_foot(struct ostream *ostream, char *name, size_t size)
{
	static const char a[] = "};\npublish_embedded(data, ";
	static const char b[] = ", \"";
	static const char c[] = "\");\n";
	char s[16];
	int n;
	write_ostream(ostream, a, sizeof(a) - 1);
#ifdef _WIN32
	n = snprintf(s, sizeof(s), "%u", size);
#else
	n = snprintf(s, sizeof(s), "%zu", size);
#endif
	write_ostream(ostream, s, n);
	write_ostream(ostream, b, sizeof(b) - 1);
	write_ostream(ostream, name, strlen(name));
	write_ostream(ostream, c, sizeof(c) - 1);
}

static unsigned char zbuf[128 * 1024]; /* FIXME: 8 + 0 * 128 * 1024 */

static unsigned char buf[16384];

static int use_zlib = 1;
b6_flag_named(use_zlib, bool, "compress");

int main(int argc, char *argv[])
{
	struct ifstream ifs;
	struct ohstream ohs;
	struct ofstream ofs;
	size_t size;
	int argf = b6_parse_command_line_flags(argc, argv, 0);
	if (argf >= argc)
		return EXIT_FAILURE;
	set_binary_mode(stdin);
	initialize_ifstream_with_fp(&ifs, stdin, 0);
	initialize_ofstream_with_fp(&ofs, stdout, 0);
	write_head(&ofs.ostream);
	initialize_ohstream_hex(&ohs, &ofs.ostream);
	if (use_zlib) {
		struct ozstream ozs;
		initialize_ozstream(&ozs, &ohs.ostream, zbuf, sizeof(zbuf));
		size = write_body(&ifs.istream, &ozs.up, buf, sizeof(buf));
		finalize_ozstream(&ozs);
	} else
		size = write_body(&ifs.istream, &ohs.ostream, buf, sizeof(buf));
	finalize_ohstream_hex(&ohs);
	write_foot(&ofs.ostream, argv[argf], size);
	finalize_ofstream(&ofs);
	finalize_ifstream(&ifs);
	return EXIT_SUCCESS;
}
