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

#ifndef PREFERENCES_H
#define PREFERENCES_H

struct b6_json;
struct b6_json_object;

struct preferences {
	char path[512];
	struct b6_json_object *root;
};

extern int initialize_pref(struct preferences*, struct b6_json*, const char*);
extern void finalize_pref(struct preferences*);

extern int load_pref(struct preferences*);
extern int save_pref(struct preferences*);

extern int get_pref_fullscreen(struct preferences*);
extern void set_pref_fullscreen(struct preferences*, int);

extern int get_pref_vsync(struct preferences*);
extern void set_pref_vsync(struct preferences*, int);

extern const void *get_pref_mode(struct preferences*, unsigned int*);
extern void set_pref_mode(struct preferences*, const void*, unsigned int);

extern int get_pref_shuffle(struct preferences*);
extern void set_pref_shuffle(struct preferences*, int);

extern const void *get_pref_game(struct preferences*, unsigned int*);
extern void set_pref_game(struct preferences*, const void*, unsigned int);

extern const void *get_pref_lang(struct preferences *self, unsigned int *size);
extern void set_pref_lang(struct preferences *self, const void *utf8,
			  unsigned int size);

extern void set_pref_level(struct preferences *self, unsigned int level,
			   const void *game_utf8, unsigned int game_size,
			   const void *mode_utf8, unsigned int mode_size);
extern unsigned int get_pref_level(const struct preferences *self,
				   const void *game_utf8,
				   unsigned int game_size,
				   const void *mode_utf8,
				   unsigned int mode_size);

#endif /* PREFERENCES_H */

