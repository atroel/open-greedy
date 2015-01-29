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

#include "menu.h"
#include "b6/observer.h"
#include "lib/log.h"

#define __notify_menu_observers(_menu, _op, _args...) \
	b6_notify_observers(&(_menu)->observers, menu_observer, _op, ##_args)

static void leave_menu_entry(struct menu *self)
{
	__notify_menu_observers(self, on_focus_out);
}

static void enter_menu_entry(struct menu *self)
{
	__notify_menu_observers(self, on_focus_in);
}

#define walk_menu_entry(self, dir, tail, first) \
	do { \
		struct menu *_self = self; \
		struct b6_dref *_dref = &_self->entry->dref; \
		struct b6_dref *_tail = tail; \
		struct b6_dref *_first = first; \
		leave_menu_entry(_self); \
		_dref = b6_list_walk(_dref, dir); \
		if (_dref == _tail) \
			_dref = _first; \
		_self->entry = b6_cast_of(_dref, struct menu_entry, dref); \
		enter_menu_entry(_self); \
	} while (0)

void previous_menu_entry(struct menu *self)
{
	walk_menu_entry(self, B6_PREV, b6_list_head(&self->entries),
			b6_list_last(&self->entries));
}

void next_menu_entry(struct menu *self)
{
	walk_menu_entry(self, B6_NEXT, b6_list_tail(&self->entries),
			b6_list_first(&self->entries));
}

void quit_menu(struct menu *self)
{
	if (self->entry != self->quit_entry) {
		leave_menu_entry(self);
		self->entry = self->quit_entry;
		enter_menu_entry(self);
	}
	select_menu_entry(self);
}

void select_menu_entry(struct menu *self)
{
	__notify_menu_observers(self, on_select);
}
