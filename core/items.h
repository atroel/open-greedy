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

#ifndef ITEMS_H
#define ITEMS_H

#include <b6/observer.h>

struct mobile;
struct place;

struct item {
	void (*dispose)(struct item*);
	struct b6_list observers;
};

struct item_observer {
	struct b6_dref dref;
	const struct item_observer_ops *ops;
};

struct item_observer_ops {
	void (*on_pacman_visit)(struct item_observer*, struct mobile*,
				struct item*);
	void (*on_ghost_visit)(struct item_observer*, struct mobile*,
			       struct item*);
};

extern void pacman_visits_item(struct mobile *m, struct item *i);

extern void ghost_visits_item(struct mobile *m, struct item *i);

struct pacgum {
	struct item item;
	unsigned int count;
};

struct super_pacgum {
	struct item item;
	unsigned int count;
};

struct teleport {
	struct item item;
	struct place *place;
	struct teleport *teleport;
};

enum bonus_type {
	JEWELS_BONUS,
	JEWEL1_BONUS,
	JEWEL2_BONUS,
	JEWEL3_BONUS,
	JEWEL4_BONUS,
	JEWEL5_BONUS,
	JEWEL6_BONUS,
	JEWEL7_BONUS,
	SINGLE_BOOSTER_BONUS,
	DOUBLE_BOOSTER_BONUS,
	SUPER_PACGUM_BONUS,
	EXTRA_LIFE_BONUS,
	PACMAN_SLOW_BONUS,
	PACMAN_FAST_BONUS,
	SHIELD_BONUS,
	DIET_BONUS,
	SUICIDE_BONUS,
	GHOSTS_BANQUET_BONUS,
	GHOSTS_WIPEOUT_BONUS,
	GHOSTS_SLOW_BONUS,
	GHOSTS_FAST_BONUS,
	ZZZ_BONUS,
	X2_BONUS,
	MINUS_BONUS,
	PLUS_BONUS,
	CASINO_BONUS,
	SURPRISE_BONUS,
	BONUS_COUNT /* not a bonus; must be the last one */
};

struct bonus {
	struct item item;
	enum bonus_type contents;
};

struct items {
	struct item empty;
	struct pacgum pacgum;
	struct super_pacgum super_pacgum;
	struct teleport teleport[2];
	struct bonus bonus;
};

static inline void dispose_item(struct item *item)
{
	if (item->dispose)
		item->dispose(item);
}

static inline struct item *clone_empty(struct items *self)
{
	return &self->empty;
}

static inline struct item *clone_pacgum(struct items *self)
{
	self->pacgum.count += 1;
	return &self->pacgum.item;
}

static inline int is_pacgum_item(const struct items *self,
				 const struct item *item)
{
	return &self->pacgum.item == item;
}

static inline struct item *clone_super_pacgum(struct items *self)
{
	self->super_pacgum.count += 1;
	return &self->super_pacgum.item;
}

static inline int is_bonus_item(const struct items *self,
				const struct item *item)
{
	return &self->bonus.item == item;
}

extern struct item *clone_bonus(struct items *self);

static inline enum bonus_type get_bonus_contents(const struct item *bonus)
{
	return b6_cast_of(bonus, struct bonus, item)->contents;
}

extern enum bonus_type unveil_bonus_contents(struct item *bonus_as_item);

extern enum bonus_type get_bonus_type_no_surprise(void);

static inline int is_super_pacgum_item(const struct items *self,
				       const struct item *item)
{
	return &self->super_pacgum.item == item;
}

static inline struct item* clone_teleport(struct items *self,
					  struct place *place)
{
	struct teleport *teleport = &self->teleport[!!self->teleport[0].place];
	if (teleport->place)
		return NULL;
	teleport->place = place;
	return &teleport->item;
}

static inline int is_teleport(const struct items *self, const struct item *item)
{
	return item == &self->teleport[0].item ||
		item == &self->teleport[1].item;
}

static inline struct place *get_teleport_destination(const struct item *item)
{
	return b6_cast_of(item, struct teleport, item)->place;
}

static inline void initialize_item_observer(struct item_observer *o,
					    const struct item_observer_ops *ops)
{
	o->ops = ops;
	b6_reset_observer(&o->dref);
}

static inline void add_item_observer(struct item *i, struct item_observer *o)
{
	b6_attach_observer(&i->observers, &o->dref);
}

static inline void del_item_observer(struct item_observer *o)
{
	b6_detach_observer(&o->dref);
}

extern void initialize_items(struct items *self);

#endif /* ITEMS_H */
