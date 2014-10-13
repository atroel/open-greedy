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

#ifndef CONSOLE_H
#define CONSOLE_H

#include "b6/registry.h"

struct console {
	struct b6_entry entry;
	const struct console_ops *ops;
	struct controller *default_controller;
	struct renderer *default_renderer;
};

struct console_ops {
	int (*open)(struct console*);
	void (*poll)(struct console*);
	void (*show)(struct console*);
	void (*close)(struct console*);
};

extern unsigned short int get_console_width();
extern unsigned short int get_console_height();
extern int get_console_fullscreen();
extern void set_console_fullscreen(int);
extern int get_console_vsync();
extern void set_console_vsync(int);

static inline int open_console(struct console *self)
{
	return self->ops->open ? self->ops->open(self) : 0;
}

static inline void close_console(struct console *self)
{
	if (self->ops->close)
		self->ops->close(self);
}

static inline void poll_console(struct console *self)
{
	if (self->ops->poll)
		self->ops->poll(self);
}

static inline void show_console(struct console *self)
{
	if (self->ops->show)
		self->ops->show(self);
}

extern struct b6_registry __console_registry;

static inline int register_console(struct console *self, const char *name)
{
	return b6_register(&__console_registry, &self->entry, name);
}

static inline void unregister_console(struct console *self)
{
	b6_unregister(&__console_registry, &self->entry);
}

static inline struct console *lookup_console(const char *name)
{
	struct b6_entry *entry = b6_lookup_registry(&__console_registry, name);
	return entry ? b6_cast_of(entry, struct console, entry) : NULL;
}

#endif /* CONSOLE_H */
