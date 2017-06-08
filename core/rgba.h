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

#ifndef RGBA_H
#define RGBA_H

struct rgba {
	unsigned char *p;
	unsigned short int w;
	unsigned short int h;
};

struct istream;
struct ostream;

extern int initialize_rgba(struct rgba *self,
			   unsigned short int w, unsigned short int h);

extern int initialize_rgba_from(struct rgba *self, const void *data,
				unsigned short int w, unsigned short int h);

extern int initialize_rgba_from_tga(struct rgba *rgba, struct istream *istream);

extern void finalize_rgba(struct rgba *self);

extern void clear_rgba(struct rgba *self, unsigned int color);

extern int write_rgba_as_tga(const struct rgba *rgba, struct ostream *ostream);

/* clipped blit without overlap test. */
extern void copy_rgba(const struct rgba *from,
		      unsigned short int x, unsigned short int y,
		      unsigned short int w, unsigned short int h,
		      struct rgba *to,
		      unsigned short int u, unsigned short int v);

extern void make_shadow_rgba(struct rgba *rgba);

#endif /* RGBA_H */
