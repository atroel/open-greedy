/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014-2017 Arnaud TROEL
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

#include "io.h"
#include "log.h"
#include <b6/allocator.h>
#include <b6/utils.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#if WITH_LZO
#include <lzo/lzo1x.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

long long int fill_membuf(struct membuf *self, struct istream *istream)
{
	unsigned long long int offset = self->off;
	long long int retval;
	for (;;) {
		if (self->off >= self->len) {
			unsigned char *tmp;
			self->len += 4096;
			tmp = b6_reallocate(self->allocator, self->buf,
					    self->len);
			if (!tmp) {
				log_w(_s("out of memory"));
				retval = -1;
				break;
			}
			self->buf = tmp;
		}
		if ((retval = read_istream(istream, self->buf + self->off,
					   self->len - self->off)) <= 0)
			break;
		self->off += retval;
	}
	if (retval < 0) {
		logf_w("error %d", retval);
		self->off = offset;
	} else
		retval = self->off - offset;
	return retval;
}

static long long int read_ifstream(struct istream *s, void *b,
				   unsigned long long int n)
{
	struct ifstream *ifs = b6_cast_of(s, struct ifstream, istream);
	if (ifs->fp)
		return fread(b, 1, n, ifs->fp);
	return read(ifs->fd, b, n);
}

static int flush_ofstream(struct ostream *s)
{
	struct ofstream *ofs = b6_cast_of(s, struct ofstream, ostream);
	if (ofs->fp)
		return fflush(ofs->fp);
	return 0;
}

static long long int write_ofstream(struct ostream *s, const void *b,
				    unsigned long long int n)
{
	struct ofstream *ofs = b6_cast_of(s, struct ofstream, ostream);
	if (ofs->fp)
		return fwrite(b, n, 1, ofs->fp) ? n : 0;
	return write(ofs->fd, b, n);
}

static void setup_ifstream(struct ifstream *self, FILE *fp, int fd,
			   int is_owner)
{
	static const struct istream_ops ifstream_ops = {
		.read = read_ifstream,
	};
	self->istream.ops = &ifstream_ops;
	self->fp = fp;
	self->fd = fd;
	self->is_owner = is_owner;
}

int initialize_ifstream_with_fd(struct ifstream *self, int fd, int is_owner)
{
	setup_ifstream(self, NULL, fd, is_owner);
	return 0;
}

int initialize_ifstream_with_fp(struct ifstream *self, FILE *fp, int is_owner)
{
	setup_ifstream(self, fp, 0, is_owner);
	return 0;
}

static FILE *platform_fopen(const char *path, const char *mode)
{
#ifndef _WIN32
	return fopen(path, mode);
#else
	wchar_t w_path[MAX_PATH];
	wchar_t w_mode[128];
	size_t len;
	len = strlen(path);
	len = MultiByteToWideChar(CP_UTF8, 0, path, len, w_path, len);
	if (!len || len >= b6_card_of(w_path))
		return NULL;
	w_path[len] = L'\0';
	len = strlen(mode);
	len = MultiByteToWideChar(CP_UTF8, 0, mode, len, w_mode, len);
	if (!len || len >= b6_card_of(w_mode))
		return NULL;
	w_mode[len] = L'\0';
	return _wfopen(w_path, w_mode);
#endif
}

int initialize_ifstream(struct ifstream *self, const char *filename)
{
	FILE *fp = platform_fopen(filename, "rb");
	if (!fp)
		return -1;
	setup_ifstream(self, fp, 0, 1);
	return 0;
}

static const struct ostream_ops ofstream_ops = {
	.write = write_ofstream,
	.flush = flush_ofstream,
};

int initialize_ofstream_with_fp(struct ofstream *self, FILE *fp, int is_owner)
{
	self->ostream.ops = &ofstream_ops;
	self->fd = 0;
	self->fp = fp;
	self->is_owner = is_owner;
	return 0;
}

int initialize_ofstream_with_fd(struct ofstream *self, int fd, int is_owner)
{
	self->ostream.ops = &ofstream_ops;
	self->fd = fd;
	self->fp = NULL;
	self->is_owner = is_owner;
	return 0;
}

int initialize_ofstream(struct ofstream *self, const char *filename)
{
	FILE *fp = platform_fopen(filename, "wb");
	if (fp == NULL)
		return -1;
	self->ostream.ops = &ofstream_ops;
	self->is_owner = 1;
	self->fp = fp;
	return 0;
}

void finalize_ifstream(struct ifstream *self)
{
	if (!self->is_owner)
		return;
	if (self->fp)
		fclose(self->fp);
	else
		close(self->fd);
}

void finalize_ofstream(struct ofstream *self)
{
	finalize_ostream(&self->ostream);
	if (!self->is_owner)
		return;
	if (self->fp)
		fclose(self->fp);
	else
		close(self->fd);
}

static long long int read_ibstream(struct istream *s, void *b,
				   unsigned long long int n)
{
	struct ibstream *ibs = b6_cast_of(s, struct ibstream, istream);
	if (n > ibs->len)
		n = ibs->len;
	memcpy(b, ibs->ptr, n);
	ibs->ptr += n;
	ibs->len -= n;
	return n;
}

void initialize_ibstream(struct ibstream *s, const void *b, unsigned long int n)
{
	static const struct istream_ops ibstream_ops = {
		.read = read_ibstream,
	};
	s->istream.ops = &ibstream_ops;
	s->ptr = b;
	s->len = n;

}

static long long int write_obstream(struct ostream *s, const void *b,
				    unsigned long long int n)
{
	struct obstream *obs = b6_cast_of(s, struct obstream, ostream);
	if (n > obs->len)
		n = obs->len;
	memcpy(obs->ptr, b, n);
	obs->ptr += n;
	obs->len -= n;
	return n;
}

void initialize_obstream(struct obstream *s, void *b, unsigned long long int n)
{
	static const struct ostream_ops obstream_ops = {
		.write = write_obstream,
	};
	s->ostream.ops = &obstream_ops;
	s->buf = b;
	s->ptr = b;
	s->len = n;
	s->allocator = NULL;
}

static long long int write_obstream_alloc(struct ostream *s, const void *b,
					  unsigned long long int n)
{
	struct obstream *obs = b6_cast_of(s, struct obstream, ostream);
	if (!obs->len) {
		unsigned long long int len = obs->ptr - obs->buf;
		char *buf = b6_reallocate(obs->allocator, obs->buf, len + 4096);
		if (!buf)
			return 0;
		obs->buf = buf;
		obs->ptr = buf + len;
		obs->len = 4096;
	}
	return write_obstream(s, b, n);
}

void initialize_obstream_alloc(struct obstream *self,
			       struct b6_allocator *allocator)
{
	static const struct ostream_ops obstream_ops = {
		.write = write_obstream_alloc,
	};
	self->ostream.ops = &obstream_ops;
	self->buf = NULL;
	self->ptr = NULL;
	self->len = 0;
	self->allocator = allocator;
}

#if WITH_LZO
static int iocstream_initialized = 0;

b6_ctor(iocstream_initialize);
static void iocstream_initialize(void)
{
	iocstream_initialized = LZO_E_OK == lzo_init();
}
#else
static int iocstream_initialized = 1;
#endif

static unsigned long long int get_lzo_size(unsigned long long int size)
{
       return size + size / 16 + 64 + 3;
}

void finalize_ocstream(struct ocstream *self)
{
	finalize_ostream(&self->ostream);
	if (self->ibuf)
		b6_deallocate(self->allocator, self->ibuf);
	if (self->obuf)
		b6_deallocate(self->allocator, self->obuf);
	if (self->cbuf)
		b6_deallocate(self->allocator, self->cbuf);
}

static int do_flush_ocstream(struct ocstream *ocs)
{
#if WITH_LZO
	int retval;
	lzo_uint csize;
	unsigned short int *optr = ocs->obuf;
	if (!ocs->used)
		return 0;
	retval = lzo1x_999_compress(ocs->ibuf, ocs->used,
				    (unsigned char*)(optr + 2), &csize,
				    ocs->cbuf);
	if (LZO_E_OK != retval)
		goto fail_lzo;
	optr[0] = csize;
	optr[1] = ocs->used;
	csize += 2 * sizeof(*optr);
	if (write_ostream(ocs->impl, ocs->obuf, csize) < csize)
		goto fail_io;
	ocs->used = 0;
	return 0;
fail_io:
	return -1;
fail_lzo:
	return -2;
#else
	if (write_ostream(ocs->impl, ocs->ibuf, ocs->used) < ocs->used)
		return -1;
	ocs->used = 0;
	return 0;
#endif
}

static int flush_ocstream(struct ostream *s) {
	struct ocstream *ocs = b6_cast_of(s, struct ocstream, ostream);
	int error = do_flush_ocstream(ocs);
	return error ? error : flush_ostream(ocs->impl);
}

static long long int write_ocstream(struct ostream *s, const void *buf,
				    unsigned long long int len)
{
	struct ocstream *ocs = b6_cast_of(s, struct ocstream, ostream);
	unsigned short int free = ocs->size - ocs->used;
	char *iptr = ocs->ibuf;
	if (len > free)
		len = free;
	memcpy(iptr + ocs->used, buf, len);
	ocs->used += len;
	free -= len;
	if (!free) {
		int retval = do_flush_ocstream(ocs);
		if (retval)
			return retval;
	}
	return len;
}

int initialize_ocstream(struct ocstream *self,
			struct ostream *ostream,
			unsigned short int size,
			struct b6_allocator *allocator)
{
	static const struct ostream_ops ocstream_ops = {
		.write = write_ocstream,
		.flush = flush_ocstream,
	};
	if (!iocstream_initialized)
		goto fail_lzo;
	self->ostream.ops = &ocstream_ops;
	self->impl = ostream;
	self->allocator = allocator;
	self->size = size;
	self->used = 0;
	self->obuf = NULL;
	self->cbuf = NULL;
	if (!(self->ibuf = b6_allocate(allocator, self->size)))
		goto fail_mem;
	if (!(self->obuf = b6_allocate(allocator, 2 * sizeof(short) +
				       get_lzo_size(self->size))))
		goto fail_mem;
#if WITH_LZO
	if (!(self->cbuf = b6_allocate(allocator, LZO1X_999_MEM_COMPRESS)))
		goto fail_mem;
#endif
	return 0;
fail_mem:
	finalize_ocstream(self);
	return -1;
fail_lzo:
	return -2;
}

void finalize_icstream(struct icstream *ics)
{
	if (ics->ibuf)
		b6_deallocate(ics->allocator, ics->ibuf);
	if (ics->obuf)
		b6_deallocate(ics->allocator, ics->obuf);
}

static int fetch_icstream(struct icstream *ics)
{
#if WITH_LZO
	int retval;
	unsigned short int *iptr = ics->ibuf;
	lzo_uint ilen = 2 * sizeof(*iptr), olen, clen;
	if (read_istream(ics->impl, iptr, ilen) < ilen)
		goto fail_io;
	ilen = *iptr++;
	olen = *iptr++;
	if (olen > ics->size)
		goto fail_size;
	if (read_istream(ics->impl, iptr, ilen) < ilen)
		goto fail_io;
	retval = lzo1x_decompress((unsigned char*)iptr, ilen, ics->obuf,
				  &clen, NULL);
	if (retval != LZO_E_OK)
		goto fail_lzo;
	ics->used = olen;
	ics->free = 0;
	return 0;
fail_io:
	return -1;
fail_lzo:
	return -2;
fail_size:
	return -3;
#else
	ics->used = read_istream(ics->impl, ics->obuf, ics->size);
	ics->free = 0;
	return 0;
#endif
}

static long long int read_icstream(struct istream *s, void *buf,
				   unsigned long long int len)
{
	struct icstream *ics = b6_cast_of(s, struct icstream, istream);
	const unsigned char *optr = ics->obuf;
	if (ics->free == ics->used) {
		int retval = fetch_icstream(ics);
		if (retval)
			return retval;
	}
	if (len > ics->used - ics->free)
		len = ics->used - ics->free;
	memcpy(buf, optr + ics->free, len);
	ics->free += len;
	return len;
}

int initialize_icstream(struct icstream *ics,
			struct istream *istream,
			unsigned long long int size,
			struct b6_allocator *allocator)
{
	static const struct istream_ops icstream_ops = {
		.read = read_icstream,
	};
	if (!iocstream_initialized)
		goto fail_lzo;
	ics->istream.ops = &icstream_ops;
	ics->impl = istream;
	ics->allocator = allocator;
	ics->size = size;
	ics->used = 0;
	ics->free = 0;
	ics->ibuf = NULL;
	ics->obuf = NULL;
	if (!(ics->ibuf = b6_allocate(allocator, 2 * sizeof(short) +
				       get_lzo_size(ics->size))))
		goto fail_mem;
	if (!(ics->obuf = b6_allocate(allocator, ics->size)))
		goto fail_mem;
	return 0;
fail_mem:
	finalize_icstream(ics);
	return -1;
fail_lzo:
	return -2;
}

static int do_flush_ohstream(struct ohstream *self)
{
	static const char hdr[] = "\t\"";
	const long long int len = self->ptr - self->buf;
	if (!len)
		return 0;
	self->ptr = self->buf;
	if (write_ostream(self->impl, hdr, sizeof(hdr) - 1) < sizeof(hdr) - 1)
		return -1;
	if (write_ostream(self->impl, self->buf, len) < len)
		return -1;
	if (self->macro) {
		static const char ftr[] = "\"\\\n";
		if (write_ostream(self->impl, ftr, sizeof(ftr) - 1) <
		    sizeof(ftr) - 1)
			return -1;
	} else {
		static const char ftr[] = "\"\n";
		if (write_ostream(self->impl, ftr, sizeof(ftr) - 1) <
		    sizeof(ftr) - 1)
			return -1;
	}
	return 0;
}

static int flush_ohstream(struct ostream *s)
{
	struct ohstream *ohs = b6_cast_of(s, struct ohstream, ostream);
	int error = do_flush_ohstream(ohs);
	return error ? error : flush_ostream(ohs->impl);
}

static long long int write_ohstream(struct ostream *s, const void *buf,
				    unsigned long long int len)
{
	struct ohstream *self = b6_cast_of(s, struct ohstream, ostream);
	const unsigned char *ptr = buf;
	long long int n = 0;
	while (len--) {
		*self->ptr++ = '\\';
		*self->ptr++ = '0' + (*ptr >> 6);
		*self->ptr++ = '0' + ((*ptr >> 3) & 7);
		*self->ptr++ = '0' + (*ptr & 7);
		ptr += 1;
		n += 1;
		if (self->ptr >= self->buf + sizeof(self->buf))
			do_flush_ohstream(self);
	}
	return n;
}

static int do_flush_ohstream_hex(struct ohstream *self)
{
	long long int wlen = self->ptr - self->buf;
	self->ptr = &self->buf[1];
	return write_ostream(self->impl, self->buf, wlen) < wlen ? -1 : 0;
}

static int flush_ohstream_hex(struct ostream *up)
{
	struct ohstream *self = b6_cast_of(up, struct ohstream, ostream);
	self->ptr -= 1;
	self->ptr[-1] = '\n';
	return do_flush_ohstream_hex(self);
}

static long long int write_ohstream_hex(struct ostream *up, const void *buf,
					unsigned long long int len)
{
	static const char digit[] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
	};
	struct ohstream *self = b6_cast_of(up, struct ohstream, ostream);
	const unsigned char *ptr = buf;
	while (len--) {
		if (self->ptr + 6 + 7 > self->buf + sizeof(self->buf)) {
			self->ptr[-1] = '\n';
			if (do_flush_ohstream_hex(self))
				return -1;
		}
		*self->ptr++ = '0';
		*self->ptr++ = 'x';
		*self->ptr++ = digit[*ptr >> 4];
		*self->ptr++ = digit[*ptr & 15];
		*self->ptr++ = ',';
		*self->ptr++ = ' ';
		ptr += 1;
	}
	return ptr - (const unsigned char*)buf;
}

void finalize_ohstream_hex(struct ohstream *self)
{
	finalize_ostream(&self->ostream);
}

int initialize_ohstream_hex(struct ohstream *self, struct ostream *ostream)
{
	static const struct ostream_ops ops = {
		.write = write_ohstream_hex,
		.flush = flush_ohstream_hex,
	};
	self->ostream.ops = &ops;
	self->impl = ostream;
	self->buf[0] = '\t';
	self->ptr = &self->buf[1];
	return 0;
}

static int do_initialize_ohstream(struct ohstream *s, struct ostream *o, int m)
{
	static const struct ostream_ops ohstream_ops = {
		.write = write_ohstream,
		.flush = flush_ohstream,
	};
	s->ostream.ops = &ohstream_ops;
	s->impl = o;
	s->macro = m;
	s->ptr = s->buf;
	return 0;
}

extern int initialize_ohstream(struct ohstream *s, struct ostream *o)
{
	return do_initialize_ohstream(s, o, 0);
}

extern int initialize_ohstream_as_macro(struct ohstream *s, struct ostream *o)
{
	return do_initialize_ohstream(s, o, 1);
}

void finalize_ohstream(struct ohstream *self)
{
	finalize_ostream(&self->ostream);
}

void *wget_ioring(const struct ioring *self, unsigned long long int *len)
{
	const unsigned long long int free = get_ioring_free(self);
	const char *limit;
	if (*len > free)
		*len = free;
	if (*len == 0)
		return NULL;
	if (self->wptr == self->rptr) {
		struct ioring *mutable = (struct ioring *)self;
		mutable->rptr = mutable->wptr = mutable->buf;
	}
	limit = self->buf + get_ioring_capacity(self);
	if (self->wptr > self->rptr && self->wptr + *len >= limit)
		*len = limit - self->wptr;
	return self->wptr;
}

int wput_ioring(struct ioring *self, unsigned long long int len)
{
	char *wptr = self->wptr + len;
	if (self->wptr >= self->rptr) {
		const char *limit = self->buf + get_ioring_capacity(self);
		if (wptr > limit)
			return -1;
		if (wptr == limit)
			wptr = self->buf;
	} else if (wptr > self->rptr)
			return -1;
	self->wptr = wptr;
	self->len += len;
	return 0;
}

long long int write_ioring(struct ostream *os, const void *buf,
			   unsigned long long int len)
{
	struct ioring *self = b6_cast_of(os, struct ioring, ostream);
	void *ptr = wget_ioring(self, &len);
	if (ptr) {
		int error;
		memcpy(ptr, buf, len);
		error = wput_ioring(self, len);
		b6_assert(!error);
		b6_unused(error);
	}
	return len;
}

/*
long long int write_ioring(struct ostream *os, const void *buf,
			   unsigned long long int len)
{
	struct ioring *self = b6_cast_of(os, struct ioring, ostream);
	const char *ptr;
	unsigned long long int count;
	const unsigned long long int free = get_ioring_free(self);
	if (len > free)
		len = free;
	if (len == 0)
		return 0;
	if (self->wptr == self->rptr)
		self->rptr = self->wptr = self->buf;
	ptr = (const char*)buf;
	count = 0;
	if (self->wptr > self->rptr &&
	    self->wptr + len >= self->buf + sizeof(self->buf)) {
		count = self->buf + sizeof(self->buf) - self->wptr;
		memcpy(self->wptr, ptr, count);
		self->wptr = self->buf;
		ptr += count;
		len -= count;
	}
	memcpy(self->wptr, ptr, len);
	self->wptr += len;
	count += len;
	self->len += count;
	return count;
}
*/

const void *rget_ioring(const struct ioring *self, unsigned long long int *len)
{
	const unsigned long long int used = get_ioring_used(self);
	if (*len >= used)
		*len = used;
	if (*len == 0)
		return NULL;
	if (self->rptr >= self->wptr &&
	    self->rptr + *len >= self->buf + sizeof(self->buf))
		*len = self->buf + sizeof(self->buf) - self->rptr;
	return self->rptr;
}

int rput_ioring(struct ioring *self, unsigned long long int len)
{
	const char *rptr = self->rptr + len;
	if (self->rptr >= self->wptr) {
		const char *limit = self->buf + get_ioring_capacity(self);
		if (rptr > limit)
			return -1;
		if (rptr == limit)
			rptr = self->buf;
	} else if (rptr > self->wptr)
			return -1;
	self->rptr = rptr;
	self->len -= len;
	return 0;
}

long long int read_ioring(struct istream *is, void *buf,
			  unsigned long long int len)
{
	struct ioring *self = b6_cast_of(is, struct ioring, istream);
	const void *ptr = rget_ioring(self, &len);
	if (ptr) {
		int error;
		memcpy(buf, ptr, len);
		error = rput_ioring(self, len);
		b6_assert(!error);
		b6_unused(error);
	}
	return len;
}

/*
long long int read_ioring(struct istream *is, void *buf,
			  unsigned long long int len)
{
	struct ioring *self = b6_cast_of(is, struct ioring, istream);
	char *ptr;
	unsigned long long int count;
	const unsigned long long int used = get_ioring_used(self);
	if (len >= used)
		len = used;
	if (len == 0)
		return 0;
	ptr = (char*)buf;
	count = 0;
	if (self->rptr >= self->wptr &&
	    self->rptr + len >= self->buf + sizeof(self->buf)) {
		count = self->buf + sizeof(self->buf) - self->rptr;
		memcpy(ptr, self->rptr, count);
		self->rptr = self->buf;
		ptr += count;
		len -= count;
	}
	memcpy(ptr, self->rptr, len);
	self->rptr += len;
	count += len;
	self->len -= count;
	return count;
}
*/

static long long int read_izstream(struct istream *up, void *buf,
				   unsigned long long int len)
{
	struct izstream *self = b6_cast_of(up, struct izstream, up);
	int retval;
	if (!self->zstream.avail_in) {
		self->zstream.next_in = self->buf;
		self->zstream.avail_in = read_istream(self->istream, self->buf,
						      self->len);
	}
	self->zstream.next_out = buf;
	self->zstream.avail_out = len;
	retval = inflate(&self->zstream, Z_SYNC_FLUSH);
	switch (retval) {
	case Z_NEED_DICT:
	case Z_DATA_ERROR:
	case Z_MEM_ERROR:
	case Z_STREAM_ERROR:
		return -1;
	case Z_STREAM_END:
	default:
		return len - self->zstream.avail_out;
	}
}

int initialize_izstream(struct izstream *self, struct istream *istream,
			void *buf, unsigned long int len)
{
	static const struct istream_ops ops = { .read = read_izstream, };
	self->istream = istream;
	self->buf = buf;
	self->len = len;
	self->zstream.avail_in = 0;
	self->zstream.zalloc = Z_NULL;
	self->zstream.zfree = Z_NULL;
	self->zstream.opaque = Z_NULL;
	if (inflateInit(&self->zstream) != Z_OK)
		return -1;
	self->up.ops = &ops;
	return 0;
}

void finalize_izstream(struct izstream *self)
{
	inflateEnd(&self->zstream);
}

static int flush_ozstream(struct ostream *up)
{
	struct ozstream *self = b6_cast_of(up, struct ozstream, up);
	unsigned long long int size;
	if ((size = self->zstream.next_out - (Bytef*)self->buf) &&
	    write_ostream(self->ostream, self->buf, size) != size)
		return -1;
	self->zstream.next_out = self->buf;
	self->zstream.avail_out = self->len;
	return 0;
}

static long long int write_ozstream(struct ostream *up, const void *buf,
				    unsigned long long int len)
{
	struct ozstream *self = b6_cast_of(up, struct ozstream, up);
	self->zstream.next_in = (void*)buf;
	self->zstream.avail_in = len;
	while (self->zstream.avail_in) {
		int retval;
		if ((retval = deflate(&self->zstream, Z_NO_FLUSH)) != Z_OK)
			return -1;
		if (!self->zstream.avail_out && (retval == flush_ozstream(up)))
			return retval;
	}
	return len;
}

int initialize_ozstream(struct ozstream *self, struct ostream *ostream,
			void *buf, unsigned long int len)
{
	static const struct ostream_ops ops = {
		.write = write_ozstream,
		.flush = flush_ozstream,
	};
	self->ostream = ostream;
	self->buf = buf;
	self->len = len;
	self->zstream.next_out = self->buf;
	self->zstream.avail_out = self->len;
	self->zstream.zalloc = Z_NULL;
	self->zstream.zfree = Z_NULL;
	self->zstream.opaque = Z_NULL;
	if (deflateInit(&self->zstream, Z_DEFAULT_COMPRESSION) != Z_OK)
		return -1;
	self->up.ops = &ops;
	return 0;
}

void finalize_ozstream(struct ozstream *self)
{
	int retval;
	self->zstream.next_in = Z_NULL;
	self->zstream.avail_in = 0;
	while ((retval = deflate(&self->zstream, Z_FINISH)) != Z_STREAM_END) {
		if (retval != Z_OK)
			break; /* FIXME: silent error */
		flush_ozstream(&self->up);
	}
	finalize_ostream(&self->up);
	flush_ostream(self->ostream);
	deflateEnd(&self->zstream);
}

int initialize_ioring(struct ioring *self)
{
	static const struct istream_ops ioring_istream_ops = {
		.read = read_ioring,
	};
	static const struct ostream_ops ioring_ostream_ops = {
		.write = write_ioring,
	};
	self->istream.ops = &ioring_istream_ops;
	self->ostream.ops = &ioring_ostream_ops;
	self->len = 0;
	self->rptr = self->wptr = self->buf;
	return 0;
}

long long int pipe_streams(struct istream *i, struct ostream *o)
{
	char buf[4096], *ptr;
	long long int size = 0;
	for (;;) {
		long long int rsize = read_istream(i, buf, sizeof(buf));
		if (rsize < 0) {
			logf_e("i/o read error #%lld", rsize);
			return rsize;
		}
		if (!rsize)
			break;
		ptr = buf;
		while (rsize) {
			long long int wsize = write_ostream(o, ptr, rsize);
			if (wsize < 0) {
				logf_e("i/o write error #%lld", wsize);
				return wsize;
			}
			rsize -= wsize;
			ptr += wsize;
			size += wsize;
		}
	}
	return size;
}
