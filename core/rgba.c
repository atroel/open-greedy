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

#include "rgba.h"

#include <string.h>

#include "b6/allocator.h"
#include "b6/assert.h"
#include "lib/std.h"
#include "lib/io.h"

int initialize_rgba(struct rgba *self,
		    unsigned short int w, unsigned short int h)
{
	self->w = w;
	self->h = h;
	self->p = b6_allocate(&b6_std_allocator, w * h * 4);
	return !!self->p ? 0 : -1;
}

int initialize_rgba_from(struct rgba *self, const void *data,
			 unsigned short int w, unsigned short int h)
{
	int retval = initialize_rgba(self, w, h);
	if (!retval)
		memcpy(self->p, data, 4 * w * h);
	return retval;
}

void finalize_rgba(struct rgba *self)
{
	b6_deallocate(&b6_std_allocator, self->p);
}

void clear_rgba(struct rgba *self, unsigned int color)
{
	unsigned int *p = (unsigned int*)self->p;
	unsigned int i = self->w * self->h;
	b6_static_assert(sizeof(*p) == 4);
	while (i--) *p++ = color;
}

void copy_rgba(const struct rgba *src,
	       unsigned short int x, unsigned short int y,
	       unsigned short int w, unsigned short int h,
	       struct rgba *dst, unsigned short int u, unsigned short int v)
{
	const unsigned char *s = &src->p[4 * (x + y * src->w)];
	unsigned char *d = &dst->p[4 * (u + v * dst->w)];
	while (h--) {
		memcpy(d, s, w * 4);
		s += 4 * src->w;
		d += 4 * dst->w;
	}
}

void make_shadow_rgba(struct rgba *rgba)
{
	unsigned int len = rgba->w * rgba->h;
	unsigned int *ptr = (unsigned int*)rgba->p;
	b6_static_assert(sizeof(*ptr) == 4);
	while (len--) {
		unsigned char a = *ptr >> 24;
		*ptr++ = a <= 0x80 ? 0x00000000 : 0x7f1f1f1f;
	}
}

struct tga_header {
	/* length of a string located after the header */
	unsigned char id_length;
	/* true iff there is a colormap */
	unsigned char colormap_type;
	/*  0: no image data included
	 *  1: uncompressed color-mapped images
	 *  2: uncompressed RGB images
	 *  3: uncompressed black and white images
	 *  9: RLE color-mapped images
	 * 10: RLE RGB images
	 * 11: compressed bland and white images
	 * 32: compressed color-mapped data using Huffman, Delta and RLE
	 * 32: compressed color-mapped data using Huffman, Delta and RLE.
	 *     4-pass quadtree-type process.
	 */
	unsigned char data_type_code;
	/* index of the first color map entry */
	unsigned short int colormap_origin;
	/* color of the color map entries */
	unsigned short int colormap_length;
	/* color map entry size: 16, 24 or 32 */
	unsigned char colormap_depth;
	/* X coordinate of the lower left corner of the image */
	unsigned short int x_origin;
	/* Y coordinate of the lower left corner of the image */
	unsigned short int y_origin;
	/* width of the image in pixels */
	unsigned short int width;
	/* height of the image in pixels */
	unsigned short int height;
	/* bits per pixel: 16, 24 or 32 */
	unsigned char bits_per_pixel;
	/* bits 3-0: number of attribute bits associated with each pixel.
	 *           0 or 1 for targa 16, 0 for 24 and 8 for 32.
	 * bit 4   : reserved
	 * bit 5   : 0 = screen origin in lower left-hand corner
	 *           1 = screen origin in upper left-hand corner
	 * bits 7-6: data storage interleaving flag
	 *           00 = non interleaved
	 *           01 = two-way (even/odd) interleaving
	 *           10 = four way interleaving
	 *           11 = reserved
	 */
	char image_descriptor;
	/* image identification field: varies */
	/* color map data: varies */
	/* image data field: varies
	 * uncompressed RGB: arrrrrgg-gggbbbb(16), BGR(24), ABGR(32)
	 * compressed RGB:
	 * - rle packet byte: 1 x 128 + repetition count minus 1
	 *   followed by a pixel - may cross scan lines
	 * - raw packet byte: 0 x 128 + pixel count minus 1
	 *   followed by n pixels
	 */
};

struct tga_to_rgba {
	int (*get)(struct tga_to_rgba*);
	void (*put)(struct tga_to_rgba*);
	struct rgba *rgba;
	struct istream *istream;
	struct tga_header header;
	unsigned char r, g, b, a;
	unsigned char *p;
	unsigned char get_count;
	unsigned short int put_count;
	unsigned char r_palette[256];
	unsigned char g_palette[256];
	unsigned char b_palette[256];
	unsigned char a_palette[256];
};

static int get_tga_10(struct tga_to_rgba *self,
		      int (*get_tga)(struct tga_to_rgba*),
		      int (*get_raw)(struct tga_to_rgba*),
		      int (*get_rle)(struct tga_to_rgba*))
{
	char rle;
	if (!read_istream(self->istream, &rle, sizeof(rle)))
		return -1;
	if (rle & 0x80) {
		int retval = get_tga(self);
		if (retval)
			return retval;
		self->get = get_rle;
	} else
		self->get = get_raw;
	self->get_count = rle & 0x7f;
	return self->get(self);
}

static int get_tga_1(struct tga_to_rgba *self)
{
	unsigned char index;
	if (!read_istream(self->istream, &index, sizeof(index)))
		return -1;
	if (index < self->header.colormap_origin)
		return -2;
	index -= self->header.colormap_origin;
	self->r = self->r_palette[index];
	self->g = self->g_palette[index];
	self->b = self->b_palette[index];
	self->a = self->a_palette[index];
	return 0;
}

static int get_tga_2_24(struct tga_to_rgba *self)
{
	unsigned char pixel[3];
	if (!read_istream(self->istream, pixel, sizeof(pixel)))
		return -1;
	self->r = pixel[2];
	self->g = pixel[1];
	self->b = pixel[0];
	return 0;
}

static int get_tga_10_24(struct tga_to_rgba*);

static int get_tga_10_24_rle(struct tga_to_rgba *self)
{
	if (!self->get_count--)
		self->get = get_tga_10_24;
	return 0;
}

static int get_tga_10_24_raw(struct tga_to_rgba *self)
{
	int retval = get_tga_2_24(self);
	return retval ? retval : get_tga_10_24_rle(self);
}

static int get_tga_10_24(struct tga_to_rgba *self)
{
	return get_tga_10(self, get_tga_2_24, get_tga_10_24_raw,
			  get_tga_10_24_rle);
}

static int get_tga_2_32(struct tga_to_rgba *self)
{
	unsigned char pixel[4];
	if (!read_istream(self->istream, pixel, sizeof(pixel)))
		return -1;
	self->r = pixel[2];
	self->g = pixel[1];
	self->b = pixel[0];
	self->a = pixel[3];
	return 0;
}

static int get_tga_10_32(struct tga_to_rgba*);

static int get_tga_10_32_rle(struct tga_to_rgba *self)
{
	if (!self->get_count--)
		self->get = get_tga_10_32;
	return 0;
}

static int get_tga_10_32_raw(struct tga_to_rgba *self)
{
	int retval = get_tga_2_32(self);
	return retval ? retval : get_tga_10_32_rle(self);
}

static int get_tga_10_32(struct tga_to_rgba *self)
{
	return get_tga_10(self, get_tga_2_32, get_tga_10_32_raw,
			  get_tga_10_32_rle);
}

static void put_tga_upper_left(struct tga_to_rgba *self)
{
	*self->p++ = self->r;
	*self->p++ = self->g;
	*self->p++ = self->b;
	*self->p++ = self->a;
}

static void put_tga_lower_left(struct tga_to_rgba *self)
{
	if (!self->put_count) {
		self->p -= 4 * self->rgba->w;
		self->put_count = self->rgba->w;
	}
	put_tga_upper_left(self);
	if (!--self->put_count)
		self->p -= 4 * self->rgba->w;
}

#define read_tga_header_field(s, h, f) \
	do { \
		static unsigned short int _swap = 0x1234; \
		b6_static_assert(sizeof((h)->f) == 1 || sizeof((h)->f) == 2); \
		if (read_istream(s, &(h)->f, sizeof((h)->f)) < sizeof((h)->f)) \
			return -1; \
		if (sizeof((h)->f) == 2 && *(char*)(&_swap) == 0x12) \
			(h)->f = ((((h)->f)&0xff)<<8) | ((((h)->f)>>8)&0xff); \
	} while (0)

static int read_tga_header(struct tga_header *h, struct istream *s)
{
	read_tga_header_field(s, h, id_length);
	read_tga_header_field(s, h, colormap_type);
	read_tga_header_field(s, h, data_type_code);
	read_tga_header_field(s, h, colormap_origin);
	read_tga_header_field(s, h, colormap_length);
	read_tga_header_field(s, h, colormap_depth);
	read_tga_header_field(s, h, x_origin);
	read_tga_header_field(s, h, y_origin);
	read_tga_header_field(s, h, width);
	read_tga_header_field(s, h, height);
	read_tga_header_field(s, h, bits_per_pixel);
	read_tga_header_field(s, h, image_descriptor);
	return 0;
}

#define write_tga_header_field(s, h, f) \
	do { \
		static unsigned short int _swap = 0x1234; \
		b6_static_assert(sizeof((h)->f) == 1 || sizeof((h)->f) == 2); \
		if (sizeof((h)->f) == 1) { \
			if (write_ostream(s, &(h)->f, 1) < 1) \
				return -1; \
		} else { \
			unsigned short int value; \
			if (*(char*)(&_swap) == 0x12) \
				value = ((((h)->f)&0xff)<<8) | \
					((((h)->f)>>8)&0xff); \
			else \
				value = (h)->f; \
			if (write_ostream(s, &value, 2) < 2) \
				return -1; \
		} \
	} while (0)

static int write_tga_header(const struct tga_header *h, struct ostream *s)
{
	write_tga_header_field(s, h, id_length);
	write_tga_header_field(s, h, colormap_type);
	write_tga_header_field(s, h, data_type_code);
	write_tga_header_field(s, h, colormap_origin);
	write_tga_header_field(s, h, colormap_length);
	write_tga_header_field(s, h, colormap_depth);
	write_tga_header_field(s, h, x_origin);
	write_tga_header_field(s, h, y_origin);
	write_tga_header_field(s, h, width);
	write_tga_header_field(s, h, height);
	write_tga_header_field(s, h, bits_per_pixel);
	write_tga_header_field(s, h, image_descriptor);
	return 0;
}

int initialize_rgba_from_tga(struct rgba *rgba, struct istream *istream)
{
	struct tga_to_rgba tga_to_rgba;
	struct tga_header *header = &tga_to_rgba.header;
	unsigned long int i;
	if (read_tga_header(header, istream))
		return -1;
	switch (header->data_type_code) {
	case 1:
		tga_to_rgba.get = get_tga_1;
		if (header->bits_per_pixel != 8)
			return -2;
		break;
	case 2:
		switch (header->bits_per_pixel) {
		case 24: tga_to_rgba.get = get_tga_2_24; break;
		case 32: tga_to_rgba.get = get_tga_2_32; break;
		default: return -2;
		}
		break;
	case 10:
		switch (header->bits_per_pixel) {
		case 24: tga_to_rgba.get = get_tga_10_24; break;
		case 32: tga_to_rgba.get = get_tga_10_32; break;
		default: return -2;
		}
		break;
	default:
		return -2;
	}
	if (header->id_length)
		skip_istream(istream, header->id_length);
	if (header->colormap_type) {
		switch (header->colormap_depth) {
		case 16: /* NOT TESTED */
			for (i = 0; i < header->colormap_length; i += 1) {
				unsigned short int p;
				if (!read_istream(istream, &p, sizeof(p)))
					return -1;
				tga_to_rgba.r_palette[i] = p >> 11;
				tga_to_rgba.g_palette[i] = (p >> 6) & 0x001f;
				tga_to_rgba.b_palette[i] = p & 0x003f;
				tga_to_rgba.a_palette[i] = 255;
			}
			break;
		case 24:
			for (i = 0; i < header->colormap_length; i += 1) {
				unsigned char p[3];
				if (!read_istream(istream, p, sizeof(p)))
					return -1;
				tga_to_rgba.r_palette[i] = p[2];
				tga_to_rgba.g_palette[i] = p[1];
				tga_to_rgba.b_palette[i] = p[0];
				tga_to_rgba.a_palette[i] = 255;
			}
			break;
		case 32: /* NOT TESTED */
			for (i = 0; i < header->colormap_length; i += 1) {
				unsigned char p[4];
				if (!read_istream(istream, p, sizeof(p)))
					return -1;
				tga_to_rgba.r_palette[i] = p[0];
				tga_to_rgba.g_palette[i] = p[1];
				tga_to_rgba.b_palette[i] = p[2];
				tga_to_rgba.a_palette[i] = p[3];
			}
			break;
		default: return -2;
		}
	}
	initialize_rgba(rgba, header->width, header->height);
	tga_to_rgba.r = tga_to_rgba.g = tga_to_rgba.b = tga_to_rgba.a = 255;
	if (header->image_descriptor & 32) {
		tga_to_rgba.p = rgba->p;
		tga_to_rgba.put = put_tga_upper_left;
	} else {
		unsigned long long int offset = 4 * rgba->w * rgba->h;
		tga_to_rgba.p = rgba->p + offset;
		tga_to_rgba.put = put_tga_lower_left;
		tga_to_rgba.put_count = 0;
	}
	tga_to_rgba.istream = istream;
	tga_to_rgba.rgba = rgba;
	for (i = rgba->w * rgba->h; i--; ) {
		int retval = tga_to_rgba.get(&tga_to_rgba);
		if (retval) {
			finalize_rgba(rgba);
			return retval;
		}
		tga_to_rgba.put(&tga_to_rgba);
	}
	return 0;
}

int write_rgba_as_tga(const struct rgba *rgba, struct ostream *ostream)
{
	struct tga_header tga_header;
	const unsigned int *ptr;
	unsigned int i;
	tga_header.id_length = 0;
	tga_header.colormap_type = 0;
	tga_header.data_type_code = 2;
	tga_header.colormap_origin = 0;
	tga_header.colormap_length = 0;
	tga_header.colormap_depth = 0;
	tga_header.x_origin = 0;
	tga_header.y_origin = 0;
	tga_header.width = rgba->w;
	tga_header.height = rgba->h;
	tga_header.bits_per_pixel = 32;
	tga_header.image_descriptor = 40;
	if (write_tga_header(&tga_header, ostream) == -1)
		return -1;
	b6_static_assert(sizeof(*ptr) == 4);
	b6_static_assert(sizeof(i) >= sizeof(rgba->w) + sizeof(rgba->h));
	for (i = rgba->w * rgba->h, ptr = (unsigned int*)rgba->p; i--; ptr++) {
		unsigned int abgr = (*ptr & 0xff00ff00) |
			((*ptr & 0x000000ff) << 16) |
			((*ptr & 0x00ff0000) >> 16);
		b6_static_assert(sizeof(*ptr) == sizeof(abgr));
		if (write_ostream(ostream, &abgr, sizeof(abgr)) < sizeof(abgr))
			return -1;
	}
	return 0;
}
