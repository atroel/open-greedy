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

#ifndef IO_H
#define IO_H

#include <b6/allocator.h>
#include <b6/utils.h>
#include <zlib.h>
#include <stdio.h>

#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
static inline void set_binary_mode(FILE *fp) { setmode(fileno(fp), O_BINARY); }
#else
static inline void set_binary_mode(FILE *fp) { }
#endif

struct istream { const struct istream_ops *ops; };

struct istream_ops {
	long long int (*read)(struct istream*, void*, unsigned long long int);
};

struct ostream { const struct ostream_ops *ops; };

struct ostream_ops {
	long long int (*write)(struct ostream*, const void*,
			       unsigned long long int);
	int (*flush)(struct ostream*);
};

static inline long long int read_istream(struct istream *s, void *b,
					 unsigned long long int n)
{
	long long int t = 0;
	unsigned char *p = b;
	while (n) {
	      long long int d = s->ops->read(s, p, n);
	      if (d <= 0)
		      break;
	      t += d;
	      n -= d;
	      p += d;
	}
	return t;
}

static inline long long int skip_istream(struct istream *s,
					 unsigned long long int n)
{
	static unsigned char buf[1024];
	long long int r, t = 0;
	while (n) {
		r = read_istream(s, buf, n < sizeof(buf) ? n : sizeof(buf));
		if (r <= 0)
			break;
		t += r;
		n -= r;
	}
	return t;
}

static inline int flush_ostream(struct ostream *s)
{
	return s->ops->flush == NULL ? 0 : s->ops->flush(s);
}

static inline long long int write_ostream(struct ostream *s, const void *b,
					  unsigned long long int n)
{
	long long int t = 0;
	const unsigned char *p = b;
	while (n) {
	      long long int d = s->ops->write(s, p, n);
	      if (d <= 0)
		      break;
	      t += d;
	      n -= d;
	      p += d;
	}
	return t;
}

static inline void finalize_ostream(struct ostream *s)
{
	flush_ostream(s);
}

extern long long int pipe_streams(struct istream *i, struct ostream *o);

struct ifstream {
	struct istream istream;
	void *fp;
	int fd;
	int is_owner;
};

struct ofstream {
	struct ostream ostream;
	void *fp;
	int fd;
	int is_owner;
};

extern int initialize_ifstream_with_fp(struct ifstream*, FILE*, int is_owner);
extern int initialize_ifstream_with_fd(struct ifstream*, int, int is_owner);
extern int initialize_ifstream(struct ifstream*, const char*);
extern void finalize_ifstream(struct ifstream*);
extern int initialize_ofstream_with_fp(struct ofstream*, FILE*, int is_owner);
extern int initialize_ofstream_with_fd(struct ofstream*, int, int is_owner);
extern int initialize_ofstream(struct ofstream*, const char*);
extern void finalize_ofstream(struct ofstream*);

struct ibstream {
	struct istream istream;
	const char *ptr;
	unsigned long int len;
};

extern void initialize_ibstream(struct ibstream*, const void*,
				unsigned long int);

struct obstream {
	struct ostream ostream;
	char *buf;
	char *ptr;
	unsigned long long int len;
	struct b6_allocator *allocator;
};

extern void initialize_obstream(struct obstream*, void*,
				unsigned long long int);

extern void initialize_obstream_alloc(struct obstream*, struct b6_allocator*);

static inline void finalize_obstream(struct obstream *self)
{
	if (self->allocator)
		b6_deallocate(self->allocator, self->buf);
}

static inline unsigned long long int get_obstream_size(const struct obstream *s)
{
	return s->ptr - s->buf;
}

struct ocstream {
	struct ostream ostream;
	struct ostream *impl;
	struct b6_allocator *allocator;
	unsigned short int size;
	unsigned short int used;
	void *ibuf;
	void *obuf;
	void *cbuf;
};

extern void finalize_ocstream(struct ocstream *self);

extern int initialize_ocstream(struct ocstream *self,
			       struct ostream *ostream,
			       unsigned short int size,
			       struct b6_allocator *allocator);

struct icstream {
	struct istream istream;
	struct istream *impl;
	struct b6_allocator *allocator;
	unsigned short int size;
	unsigned short int used;
	unsigned short int free;
	void *ibuf;
	void *obuf;
	void *cbuf;
};

extern void finalize_icstream(struct icstream *self);

extern int initialize_icstream(struct icstream *self,
			       struct istream *istream,
			       unsigned long long int size,
			       struct b6_allocator *allocator);

struct ohstream {
	struct ostream ostream;
	struct ostream *impl;
	int macro;
	char buf[80];
	char *ptr;
};

extern void finalize_ohstream(struct ohstream*);

extern int initialize_ohstream(struct ohstream*, struct ostream*);

extern int initialize_ohstream_as_macro(struct ohstream*, struct ostream*);

extern void finalize_ohstream_hex(struct ohstream*);

extern int initialize_ohstream_hex(struct ohstream*, struct ostream*);

struct ioring {
	struct istream istream;
	struct ostream ostream;
	char buf[8192];
	const char *rptr;
	char *wptr;
	int len;
};

extern int initialize_ioring(struct ioring *self);

static inline unsigned long long int get_ioring_capacity(const struct ioring *r)
{
	return sizeof(r->buf);
}

static inline unsigned long long int get_ioring_used(const struct ioring *r)
{
	return r->len;
}

static inline unsigned long long int get_ioring_free(const struct ioring *r)
{
	return get_ioring_capacity(r) - get_ioring_used(r);
}

void *wget_ioring(const struct ioring*, unsigned long long int *len);
int wput_ioring(struct ioring*, unsigned long long int len);
const void *rget_ioring(const struct ioring*, unsigned long long int *len);
int rput_ioring(struct ioring*, unsigned long long int len);

/* decompress / inflate */
struct izstream {
	struct istream up;
	struct istream *istream;
	z_stream zstream;
	void *buf;
	unsigned long int len;
};

extern int initialize_izstream(struct izstream *self, struct istream *istream,
			       void *buf, unsigned long int len);

extern void finalize_izstream(struct izstream *self);

struct izbstream {
	struct ibstream ibs;
	struct izstream izs;
	unsigned char buf[2048];
};

static inline struct istream * izbstream_as_istream(struct izbstream *self)
{
	return &self->izs.up;
}

static inline struct izbstream *istream_as_izbstream(struct istream *up)
{
	struct izstream *izs = b6_cast_of(up, struct izstream, up);
	return b6_cast_of(izs, struct izbstream, izs);
}

static inline int initialize_izbstream(struct izbstream *self,
				       const void *buf, unsigned int len)
{
	initialize_ibstream(&self->ibs, buf, len);
	return initialize_izstream(&self->izs, &self->ibs.istream, &self->buf,
				   sizeof(self->buf));
}

static inline void finalize_izbstream(struct izbstream *self)
{
	finalize_izstream(&self->izs);
}

/* compress / deflate */
struct ozstream {
	struct ostream up;
	struct ostream *ostream;
	z_stream zstream;
	void *buf;
	unsigned long int len;
};

extern int initialize_ozstream(struct ozstream *self, struct ostream *ostream,
			       void *buf, unsigned long int len);

extern void finalize_ozstream(struct ozstream *self);

struct membuf {
	struct b6_allocator *allocator;
	unsigned char *buf;
	unsigned long long int off, len;
};

static inline int initialize_membuf(struct membuf *self,
				    struct b6_allocator *allocator)
{
	self->allocator = allocator;
	self->off = self->len = 0;
	self->buf = NULL;
	return 0;
}

static inline void finalize_membuf(struct membuf *self)
{
	b6_deallocate(self->allocator, self->buf);
}

extern long long int fill_membuf(struct membuf*, struct istream*);

#endif /*IO_H*/
