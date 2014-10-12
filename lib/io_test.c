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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "io.h"

#include "lib/std.h"
#include "b6/extra/test.h"
#include "b6/cmdline.h"

static long long int crunch(unsigned short int bsize,
			    const char *ibuf, size_t ilen,
			    char *obuf, size_t olen,
			    unsigned long long int *osize)
{
	struct obstream ob;
	struct ocstream oc;
	long long int retval;
	initialize_obstream(&ob, obuf, olen);
	b6_expect(!initialize_ocstream(&oc, &ob.ostream, bsize,
				       &b6_std_allocator));
	retval = write_ostream(&oc.ostream, ibuf, ilen);
	finalize_ocstream(&oc);
	if (osize)
		*osize = (char*)ob.ptr - obuf;
	if (retval)
		return retval;
	return 0;
}

static long long int decrunch(unsigned short int bsize,
			      const char *ibuf, size_t ilen,
			      char *obuf, size_t olen)
{
	struct ibstream ib;
	struct icstream ic;
	long long int retval;
	initialize_ibstream(&ib, ibuf, ilen);
	b6_expect(!initialize_icstream(&ic, &ib.istream, bsize,
				       &b6_std_allocator));
	retval = read_istream(&ic.istream, obuf, olen);
	finalize_icstream(&ic);
	return retval;
}

static void cstream()
{
	static const int bsize = 11;
	const char s1[] = "Coin coin fait le canard.";
	char s2[256];
	char s3[sizeof(s1)];
	long long int ilen, olen;
	unsigned long long int osize;

	ilen = crunch(bsize, s1, sizeof(s1), s2, sizeof(s2), &osize);
	olen = decrunch(bsize, s2, osize, s3, sizeof(s3));

	b6_expect(olen == ilen);
	b6_expect(!strcmp(s1, s3));
}
b6_test(cstream);

static void ioring()
{
	struct ioring io;
	struct istream *is = &io.istream;
	struct ostream *os = &io.ostream;
	char a[sizeof(io.buf)];
	char b[sizeof(a)];
	static const char s[] = "test";
	int i;
	struct timespec timespec;
	int retval = clock_gettime(CLOCK_MONOTONIC_RAW, &timespec);
	b6_check(!retval);
	srand(timespec.tv_sec * 1000 * 1000 + timespec.tv_nsec / 1000);
	for (i = 0; i < sizeof(a); i += 1)
		a[i] = rand();

	b6_check(!initialize_ioring(&io));

	/* reset, write and read */
	b6_expect(!get_ioring_used(&io));
	io.rptr = io.wptr = &io.buf[42];
	b6_expect(write_ostream(os, s, sizeof(s)) == sizeof(s));
	b6_expect(get_ioring_used(&io) == 5);
	b6_expect(read_istream(is, b, sizeof(s)) == sizeof(s));
	b6_expect(!get_ioring_used(&io));
	b6_expect(!strcmp(s, b));
	b6_expect(io.wptr == io.rptr);
	b6_expect(io.wptr == &io.buf[sizeof(s)]);

	/* read in the middle */
	b6_expect(!get_ioring_used(&io));
	b6_expect(write_ostream(os, s, sizeof(s)) == sizeof(s));
	b6_expect(read_istream(is, b, 2) == 2);
	b6_expect(read_istream(is, &b[2], sizeof(s)) == sizeof(s) - 2);
	b6_expect(!strcmp(s, b));

	/* fractioned read */
	b6_expect(!get_ioring_used(&io));
	b6_expect(write_ostream(os, s, sizeof(s)) == sizeof(s));
	b6_expect(write_ostream(os, a, sizeof(a)) == sizeof(a) - sizeof(s));
	b6_expect(!get_ioring_free(&io));
	b6_expect(read_istream(is, b, sizeof(s)) == sizeof(s));
	b6_expect(!strcmp(s, b));
	b6_expect(write_ostream(os, a + sizeof(a) - sizeof(s), sizeof(s)) ==
		  sizeof(s));
	b6_expect(!get_ioring_free(&io));
	b6_expect(read_istream(is, b, sizeof(b) - 2) == sizeof(b) - 2);
	b6_expect(read_istream(is, b + sizeof(b) - 2, 2) == 2);
	b6_expect(!memcmp(a, b, sizeof(a)));

	/* fractioned write */
	b6_expect(!get_ioring_used(&io));
	b6_expect(write_ostream(os, s, sizeof(s)) == sizeof(s));
	b6_expect(read_istream(is, b, 2) == 2);
	b6_expect(get_ioring_used(&io) + 2 == sizeof(s));
	b6_expect(write_ostream(os, a, sizeof(a)) == sizeof(a) - sizeof(s) + 2);
	b6_expect(read_istream(is, &b[2], sizeof(s) - 2) == sizeof(s) - 2);
	b6_expect(!strcmp(s, b));
	b6_expect(write_ostream(os, a + sizeof(a) - sizeof(s) + 2,
				sizeof(s) - 2) == sizeof(s) - 2);
	b6_expect(!get_ioring_free(&io));
	b6_expect(io.wptr == io.rptr);
	b6_expect(io.wptr > io.buf);
	b6_expect(read_istream(is, b, sizeof(b)) == sizeof(b));
	b6_expect(!memcmp(a, b, sizeof(a)));
}
b6_test(ioring);

int main(int argc, char *argv[])
{
	b6_flag_parse_command_line(argc, argv, 1);
	return b6_test_run_all(argv[0]);
}
