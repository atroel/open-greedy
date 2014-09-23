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

#include "credits_phase.h"

#include "lib/init.h"

#include "console.h"
#include "controller.h"
#include "engine.h"
#include "lang.h"
#include "mixer.h"
#include "data.h"
#include "toolkit.h"

static const char *credits_skin = "default";

struct credits_phase_pacman {
	struct renderer_observer renderer_observer;
	struct renderer *renderer;
	struct renderer_tile *tile;
	const struct b6_clock *clock;
	float vx, vy;
	float xmin, ymin, xmax, ymax;
	unsigned long long int t;
};

static void on_render_credits_phase_pacman(struct renderer_observer *observer)
{
	struct credits_phase_pacman *self =
		b6_cast_of(observer, struct credits_phase_pacman,
			   renderer_observer);
	unsigned long long int t = b6_get_clock_time(self->clock);
	struct renderer_base *base = self->tile->parent;
	float dt = self->t ? (float)(t - self->t) : 0;
	float x = base->x + self->vx * dt;
	float y = base->y + self->vy * dt;
	self->t = t;
	if ((x <= self->xmin && self->vx < 0) ||
	    (x >= self->xmax && self->vx > 0))
		self->vx = -self->vx;
	if ((y <= self->ymin && self->vy < 0) ||
	    (y >= self->ymax && self->vy > 0))
		self->vy = -self->vy;
	move_renderer_base(base, x, y);
}

static int initialize_credits_phase_pacman(struct credits_phase_pacman *self,
					   struct renderer *renderer,
					   const struct b6_clock *clock,
					   float xmin, float ymin,
					   float xmax, float ymax) {
	static const struct renderer_observer_ops renderer_observer_ops = {
		.on_render = on_render_credits_phase_pacman,
	};
	struct renderer_base *base = NULL;
	struct renderer_texture *texture = NULL;
	if (!(texture = make_texture(renderer, credits_skin,
				     CREDITS_PACMAN_DATA_ID)))
		goto bail_out;
	if (!(base = create_renderer_base(renderer, get_renderer_base(renderer),
					  "pacman", 192, 112)))
		goto bail_out;
	if (!(self->tile = create_renderer_tile(renderer, base, 0, 0, 192, 192,
						texture)))
		goto bail_out;
	self->renderer = renderer;
	self->clock = clock;
	self->xmin = xmin - self->tile->w / 2;
	self->ymin = ymin - self->tile->h / 2;
	self->xmax = xmax - self->tile->w / 2;
	self->ymax = ymax - self->tile->h / 2;
	self->vx = 2e-4;
	self->vy = 2e-4;
	self->t = 0;
	add_renderer_observer(renderer, setup_renderer_observer(
			&self->renderer_observer, "credits_pacman",
			&renderer_observer_ops));
	return 0;
bail_out:
	destroy_renderer_texture(texture);
	destroy_renderer_base(base);
	self->tile = NULL;
	return -1;
}

static void finalize_credits_phase_pacman(struct credits_phase_pacman *self)
{
	if (!self->tile)
		return;
	del_renderer_observer(&self->renderer_observer);
	destroy_renderer_texture(self->tile->texture);
	destroy_renderer_base(self->tile->parent);
}

struct credits_phase {
	struct phase up;
	struct credits_skin *skin;
	struct fixed_font font;
	struct renderer_tile *background;
	struct credits_phase_pacman pacman;
	struct renderer_base *labels_base;
	struct toolkit_label lines[30];
	int music;
	struct controller_observer controller_observer;
	struct phase *next;
};

static void on_key_pressed(struct controller_observer *observer,
			   enum controller_key key)
{
	struct credits_phase *self = b6_cast_of(observer, struct credits_phase,
						controller_observer);
	if (key == CTRLK_RETURN || key == CTRLK_ESCAPE)
		self->next = lookup_phase("menu");
}

static int credits_phase_init(struct phase *up, struct engine *engine)
{
	static const struct controller_observer_ops controller_observer_ops = {
		.on_key_pressed = on_key_pressed,
	};
	struct credits_phase *self = b6_cast_of(up, struct credits_phase, up);
	struct renderer *renderer = get_engine_renderer(engine);
	struct renderer_base *root = get_renderer_base(renderer);
	struct data_entry *data;
	struct istream *istream;
	unsigned short int font_w, font_h;
	int i;
	if (make_font(&self->font, credits_skin, CREDITS_FONT_DATA_ID))
		return -1;
	font_w = get_fixed_font_width(&self->font);
	font_h = get_fixed_font_height(&self->font);
	if ((self->background = create_renderer_tile(renderer, root, 0, 0,
						     640, 480, NULL)))
		set_renderer_tile_texture(self->background, make_texture(
			renderer, credits_skin, CREDITS_BACKGROUND_DATA_ID));
	initialize_credits_phase_pacman(&self->pacman, renderer,
					engine->clock, 0, 0, 640, 480);
	self->labels_base = create_renderer_base_or_die(renderer, root,
							"labels", 0, 0);
	for (i = 0; i < b6_card_of(self->lines); i += 1) {
		const unsigned short int u = 2 + 52 * font_w, v = 2 + font_h;
		const float x = 8, y = i * 16;
		const float w = 2 + 52 * 12, h = 2 + 16;
		const char *text = engine->lang->credits.lines[i];
		initialize_toolkit_label(&self->lines[i], renderer, &self->font,
					 u, v, self->labels_base, x, y, w, h);
		enable_toolkit_label_shadow(&self->lines[i]);
		if (text)
			set_toolkit_label(&self->lines[i], text);
	}
	if ((data = lookup_data(credits_skin, audio_data_type,
				CREDITS_MUSIC_DATA_ID)) &&
	    (istream = get_data(data))) {
		self->music = engine->mixer->ops->load_music_from_stream(
			engine->mixer, istream);
		put_data(data, istream);
		play_music(engine->mixer);
		self->music = 1;
	} else
		self->music = 0;
	add_controller_observer(get_engine_controller(engine),
				setup_controller_observer(
					&self->controller_observer,
					&controller_observer_ops));
	self->next = up;
	return 0;
}

static void credits_phase_exit(struct phase *up, struct engine *engine)
{
	struct credits_phase *self = b6_cast_of(up, struct credits_phase, up);
	int i;
	del_controller_observer(&self->controller_observer);
	if (self->music) {
		stop_music(engine->mixer);
		unload_music(engine->mixer);
	}
	for (i = 0; i < b6_card_of(self->lines); i += 1)
		finalize_toolkit_label(&self->lines[i]);
	destroy_renderer_base(self->labels_base);
	finalize_credits_phase_pacman(&self->pacman);
	finalize_fixed_font(&self->font);
	if (self->background) {
		destroy_renderer_texture(self->background->texture);
		destroy_renderer_tile(self->background);
	}
}

static struct phase *credits_phase_exec(struct phase *up, struct engine *engine)
{
	return b6_cast_of(up, struct credits_phase, up)->next;
}

static int credits_phase_ctor(void)
{
	static const struct phase_ops ops = {
		.init = credits_phase_init,
		.exit = credits_phase_exit,
		.exec = credits_phase_exec,
	};
	static struct credits_phase credits_phase;
	return register_phase(&credits_phase.up, "credits", &ops);
}
register_init(credits_phase_ctor);
