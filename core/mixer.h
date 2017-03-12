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

#ifndef MIXER_H
#define MIXER_H

#include "b6/registry.h"

struct sample {
	const struct sample_ops *ops;
};

struct sample_ops {
	void (*dtor)(struct sample*);
};

struct mixer {
	struct b6_entry entry;
	const struct mixer_ops *ops;
};

struct istream;

struct mixer_ops {
	int (*open)(struct mixer*);
	void (*close)(struct mixer*);
	void (*suspend)(struct mixer*);
	void (*resume)(struct mixer*);
	struct sample *(*load_sample)(struct mixer*, const char*);
	struct sample *(*make_sample)(struct mixer*, const void*,
				      unsigned long long int);
	struct sample *(*load_sample_from_stream)(struct mixer*,
						  struct istream*);
	void (*play_sample)(struct mixer*, struct sample*);
	void (*try_play_sample)(struct mixer*, struct sample*);
	void (*stop_sample)(struct mixer*, struct sample*);
	int (*load_music)(struct mixer*, const char*);
	int (*load_music_from_mem)(struct mixer*, const void*,
				   unsigned long long int);
	int (*load_music_from_stream)(struct mixer*, struct istream*);
	void (*unload_music)(struct mixer*);
	void (*play_music)(struct mixer*);
	void (*stop_music)(struct mixer*);
	void (*set_music_pos)(struct mixer*, int pos);
};

static inline void setup_mixer(struct mixer *self, const struct mixer_ops *ops)
{
	self->ops = ops;
}

static inline void initialize_mixer(struct mixer *self,
				    const struct mixer_ops *ops)
{
	setup_mixer(self, ops);
}

static inline int open_mixer(struct mixer *self)
{
	return self->ops->open ? self->ops->open(self) : 0;
}

static inline void close_mixer(struct mixer *self)
{
	if (self->ops->close)
		self->ops->close(self);
}

static inline struct sample *load_sample(struct mixer *self, const char *path)
{
	return self->ops->load_sample(self, path);
}

static inline struct sample *make_sample(struct mixer *self, const void *buf,
					 unsigned long long int len)
{
	return self->ops->make_sample(self, buf, len);
}

static inline void play_sample(struct mixer *self, struct sample *sample)
{
	if (sample)
		self->ops->play_sample(self, sample);
}

static inline void try_play_sample(struct mixer *self, struct sample *sample)
{
	if (sample)
		self->ops->try_play_sample(self, sample);
}

static inline void stop_sample(struct mixer *self, struct sample *sample)
{
	if (sample)
		self->ops->stop_sample(self, sample);
}

static inline void destroy_sample(struct sample *sample)
{
	if (sample)
		sample->ops->dtor(sample);
}

static inline int load_music(struct mixer *self, const char *path)
{
	return self->ops->load_music(self, path);
}

static inline void unload_music(struct mixer *self)
{
	return self->ops->unload_music(self);
}

static inline void play_music(struct mixer *self)
{
	self->ops->play_music(self);
}

static inline void stop_music(struct mixer *self)
{
	self->ops->stop_music(self);
}

static inline void suspend_mixer(struct mixer *self)
{
	self->ops->suspend(self);
}

static inline void resume_mixer(struct mixer *self)
{
	self->ops->resume(self);
}

static inline void set_music_pos(struct mixer *self, int pos)
{
	self->ops->set_music_pos(self, pos);
}

extern struct b6_registry __mixer_registry;

static inline int register_mixer(struct mixer *self, const struct b6_utf8 *id)
{
	return b6_register(&__mixer_registry, &self->entry, id);
}

static inline void unregister_mixer(struct mixer *self)
{
	b6_unregister(&__mixer_registry, &self->entry);
}

static inline struct mixer *lookup_mixer(const struct b6_utf8 *id)
{
	struct b6_entry *entry = b6_lookup_registry(&__mixer_registry, id);
	return entry ? b6_cast_of(entry, struct mixer, entry) : NULL;
}

#endif /* MIXER_H */
