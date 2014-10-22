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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <b6/observer.h>

struct controller {
	struct b6_list observers;
};

enum controller_key {
	CTRLK_UP,
	CTRLK_DOWN,
	CTRLK_LEFT,
	CTRLK_RIGHT,
	CTRLK_ESCAPE,
	CTRLK_RETURN,
	CTRLK_LSHIFT,
	CTRLK_RSHIFT,
	CTRLK_LCTRL,
	CTRLK_RCTRL,
	CTRLK_DELETE,
	CTRLK_BACKSPACE,
	CTRLK_1,
	CTRLK_2,
	CTRLK_3,
	CTRLK_4,
	CTRLK_5,
	CTRLK_6,
	CTRLK_7,
	CTRLK_8,
	CTRLK_9,
	CTRLK_0,
	CTRLK_MINUS,
	CTRLK_EQUAL,
	CTRLK_q,
	CTRLK_w,
	CTRLK_e,
	CTRLK_r,
	CTRLK_t,
	CTRLK_y,
	CTRLK_u,
	CTRLK_i,
	CTRLK_o,
	CTRLK_p,
	CTRLK_a,
	CTRLK_s,
	CTRLK_d,
	CTRLK_f,
	CTRLK_g,
	CTRLK_h,
	CTRLK_j,
	CTRLK_k,
	CTRLK_l,
	CTRLK_z,
	CTRLK_x,
	CTRLK_c,
	CTRLK_v,
	CTRLK_b,
	CTRLK_n,
	CTRLK_m,
	CTRLK_PERIOD,
	CTRLK_COMMA,
	CTRLK_ENTER,
	CTRLK_UNKNOWN,
};

struct controller_observer {
	struct b6_dref dref;
	const struct controller_observer_ops *ops;
};

struct controller_observer_ops {
	void (*on_key_pressed)(struct controller_observer*,
			       enum controller_key);
	void (*on_key_released)(struct controller_observer*,
				enum controller_key);
	void (*on_text_input)(struct controller_observer*, unsigned short int);
	void (*on_quit)(struct controller_observer*);
	void (*on_focus_in)(struct controller_observer*);
	void (*on_focus_out)(struct controller_observer*);
};

extern void __notify_controller_key_pressed(struct controller*,
					    enum controller_key);

extern void __notify_controller_key_released(struct controller*,
					     enum controller_key);

extern void __notify_controller_text_input(struct controller *self,
					   unsigned short int unicode);

static inline void notify_controller_text_input(struct controller *self,
						unsigned short int unicode)
{
	if (unicode)
		__notify_controller_text_input(self, unicode);
}

extern void __notify_controller_quit(struct controller *self);

extern void __notify_controller_focus_in(struct controller *self);

extern void __notify_controller_focus_out(struct controller *self);

static inline struct controller_observer *setup_controller_observer(
	struct controller_observer *self,
	const struct controller_observer_ops *ops)
{
	self->ops = ops;
	b6_reset_observer(&self->dref);
	return self;
}

static inline void add_controller_observer(struct controller *self,
					   struct controller_observer *observer)
{
	b6_attach_observer(&self->observers, &observer->dref);
}

static inline void del_controller_observer(struct controller_observer *observer)
{
	b6_detach_observer(&observer->dref);
}

static inline struct controller *setup_controller(struct controller *self)
{
	b6_list_initialize(&self->observers);
	return self;
}

#endif /* CONTROLLER_H */
