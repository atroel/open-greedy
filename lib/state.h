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

#ifndef STATE_H
#define STATE_H

#include <b6/list.h>
#include <b6/assert.h>

struct state_queue {
	struct b6_list list;
};

struct state {
	struct b6_dref dref;
	const struct state_ops *ops;
};

struct state_ops {
	void (*on_enqueue)(struct state*);
	void (*on_dequeue)(struct state*);
	void (*on_promote)(struct state*);
	void (*on_demote)(struct state*);
};

static inline void reset_state_queue(struct state_queue *self)
{
	b6_list_initialize(&self->list);
}

static inline void reset_state(struct state *self, const struct state_ops *ops)
{
	b6_assert(ops);
	self->dref.ref[0] = (void*)0;
	self->ops = ops;
}

static inline int is_queued_state(const struct state *self)
{
	return !!self->dref.ref[0];
}

static inline int is_current_state(const struct state *self,
				   const struct state_queue *state_queue)
{
	b6_assert(is_queued_state(self));
	return b6_list_first(&state_queue->list) == &self->dref;
}

static inline struct state *current_state(const struct state_queue *state_queue)
{
	struct b6_dref *dref = b6_list_first(&state_queue->list);
	if (dref == b6_list_tail(&state_queue->list))
		return NULL;
	return b6_cast_of(dref, struct state, dref);
}

static inline void __enqueue_state(struct state *self,
				   struct state_queue *state_queue)
{
	struct state *current = current_state(state_queue);
	b6_assert(!is_queued_state(self));
	b6_list_add_first(&state_queue->list, &self->dref);
	if (self->ops->on_enqueue)
		self->ops->on_enqueue(self);
	if (current && current->ops->on_demote)
		current->ops->on_demote(current);
}

static inline int enqueue_state(struct state *self,
				struct state_queue *state_queue)
{
	int retval = -is_queued_state(self);
	if (!retval)
		__enqueue_state(self, state_queue);
	return retval;
}

static inline void __dequeue_state(struct state *self,
				   struct state_queue *state_queue)
{
	struct state *current;
	b6_assert(is_queued_state(self));
	if (self->ops->on_dequeue)
		self->ops->on_dequeue(self);
	b6_list_del(&self->dref);
	self->dref.ref[0] = (void*)NULL;
	if ((current = current_state(state_queue)) && current->ops->on_promote)
		current->ops->on_promote(current);
}

static inline int dequeue_state(struct state *self,
				struct state_queue *state_queue)
{
	int retval = -!is_queued_state(self);
	if (!retval)
		__dequeue_state(self, state_queue);
	return retval;
}

#endif /* STATE_H */
