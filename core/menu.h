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

#ifndef MENU_H
#define MENU_H

#include <b6/list.h>
#include <b6/observer.h>
#include <b6/utf8.h>

struct menu_entry {
	struct b6_dref dref;
	struct b6_utf8 utf8;
	void *cookie;
};

struct menu {
	struct b6_list observers;
	struct b6_list entries;
	struct menu_entry *entry;
	struct menu_entry *quit_entry;
};

struct menu_iterator {
	struct menu *menu;
	struct menu_entry *entry;
};

static inline void setup_menu_iterator(struct menu_iterator *self,
				       struct menu *menu)
{
	struct b6_dref *dref = b6_list_first(&menu->entries);
	self->entry = b6_cast_of(dref, struct menu_entry, dref);
	self->menu = menu;
}

static inline int menu_iterator_has_next(const struct menu_iterator *self)
{
	return &self->entry->dref != b6_list_tail(&self->menu->entries);
}

static inline struct menu_entry *menu_iterator_next(struct menu_iterator *self)
{
	struct menu_entry *entry = self->entry;
	struct b6_dref *dref = b6_list_walk(&entry->dref, B6_NEXT);
	self->entry = b6_cast_of(dref, struct menu_entry, dref);
	return entry;
}

struct menu_observer {
	struct b6_dref dref;
	const struct menu_observer_ops *ops;
};

struct menu_observer_ops {
	void (*on_focus_in)(struct menu_observer*);
	void (*on_focus_out)(struct menu_observer*);
	void (*on_select)(struct menu_observer*);
};

static inline void initialize_menu(struct menu *self)
{
	b6_list_initialize(&self->observers);
	b6_list_initialize(&self->entries);
}

static inline struct menu_entry *get_current_menu_entry(const struct menu *self)
{
	return self->entry;
}

extern void previous_menu_entry(struct menu *self);

extern void next_menu_entry(struct menu *self);

extern void select_menu_entry(struct menu *self);

extern void quit_menu(struct menu *self);

static inline struct menu_observer *setup_menu_observer(
	struct menu_observer *self, const struct menu_observer_ops *ops)
{
	self->ops = ops;
	b6_reset_observer(&self->dref);
	return self;
}

static inline void add_menu_observer(struct menu *m, struct menu_observer *o)
{
	b6_attach_observer(&m->observers, &o->dref);
}

static inline void del_menu_observer(struct menu_observer *o)
{
	b6_detach_observer(&o->dref);
}

static inline void add_menu_entry(struct menu *m, struct menu_entry *e)
{
	b6_list_add_last(&m->entries, &e->dref);
	m->entry =
		b6_cast_of(b6_list_first(&m->entries), struct menu_entry, dref);
}

static inline void set_quit_entry(struct menu *m, struct menu_entry *e)
{
	m->quit_entry = e;
}

#endif /* MENU_H */
