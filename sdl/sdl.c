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

#include <SDL.h>
#include <SDL_mixer.h>
#include <GL/gl.h>
#define BUILDING_STATIC
#include <xmp.h>
#undef BUILDING_STATIC
#include <b6/clock.h>
#include <b6/cmdline.h>
#include <string.h>

#include "core/console.h"
#include "core/controller.h"
#include "core/mixer.h"
#include "core/renderer.h"
#include "gl/gl_renderer.h"
#include "lib/init.h"
#include "lib/io.h"
#include "lib/log.h"
#include "lib/std.h"

static const char *sdl_accel = "1"; /* 0, 1, opengl, direct3d, ... */
b6_flag(sdl_accel, string);

static const char *sdl_scale = "linear"; /* nearest, linear or best */
b6_flag(sdl_scale, string);

static unsigned int sdl_sleep = 15;
b6_flag(sdl_sleep, uint);

static Uint32 flags = SDL_WINDOW_RESIZABLE;

static int sdl_count = 0;

static void finalize_sdl()
{
	if (!sdl_count)
	       return;
	if (!--sdl_count)
		SDL_Quit();
}

static int initialize_sdl()
{
	int error = 0;
	if (sdl_count++)
		return 0;
	if ((error = SDL_Init(0))) {
		log_e("SDL_Init: %s\n.", SDL_GetError());
		sdl_count = 0;
	}
	return error;
}

static SDL_Window *window = NULL;

void finalize_sdl_video()
{
	if (window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}
	SDL_ShowCursor(SDL_ENABLE);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	finalize_sdl();
}

static int initialize_sdl_video()
{
	if (initialize_sdl())
		return -1;
	if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		log_e("%s", SDL_GetError());
		return -1;
	}
	return 0;
}

static int get_sdl_video_size(unsigned short int *w, unsigned short int *h)
{
	int ww, wh;
	if (!window)
		return -1;
	SDL_GetWindowSize(window, &ww, &wh);
	*w = ww;
	*h = wh;
	return 0;
}

static int set_sdl_video_size(unsigned short int w, unsigned short int h)
{
	if (window)
		SDL_DestroyWindow(window);
	if (get_console_fullscreen()) {
		SDL_ShowCursor(SDL_DISABLE);
		flags |= SDL_WINDOW_FULLSCREEN;
	} else
		flags &= ~SDL_WINDOW_FULLSCREEN;
	if (!w || !h) {
		w = 640;
		h = 480;
		if (flags & SDL_WINDOW_FULLSCREEN) {
			SDL_DisplayMode mode;
			if (!SDL_GetDesktopDisplayMode(0, &mode)) {
				w = mode.w;
				h = mode.h;
			} else
				log_e("SDL_GetDesktopDisplayMode: %s",
				      SDL_GetError());
		}
	}
	log_i("%ux%u", w, h);
	window = SDL_CreateWindow("greedy",
				  SDL_WINDOWPOS_CENTERED,
				  SDL_WINDOWPOS_CENTERED,
				  w, h, flags);
	if (!window)
		log_p("SDL_CreateWindow: %s", SDL_GetError());
	SDL_Delay(50);
	return 0;
}

static enum controller_key sdl_to_controller_key(int sym)
{
	switch (sym) {
	case SDLK_UP: return CTRLK_UP;
	case SDLK_DOWN: return CTRLK_DOWN;
	case SDLK_LEFT: return CTRLK_LEFT;
	case SDLK_RIGHT: return CTRLK_RIGHT;
	case SDLK_ESCAPE: return CTRLK_ESCAPE;
	case SDLK_RETURN: return CTRLK_RETURN;
	case SDLK_LSHIFT: return CTRLK_LSHIFT;
	case SDLK_RSHIFT: return CTRLK_RSHIFT;
	case SDLK_LCTRL: return CTRLK_LCTRL;
	case SDLK_RCTRL: return CTRLK_RCTRL;
	case SDLK_BACKSPACE: return CTRLK_BACKSPACE;
	case SDLK_DELETE: return CTRLK_DELETE;
	case SDLK_1: return CTRLK_1;
	case SDLK_2: return CTRLK_2;
	case SDLK_3: return CTRLK_3;
	case SDLK_4: return CTRLK_4;
	case SDLK_5: return CTRLK_5;
	case SDLK_6: return CTRLK_6;
	case SDLK_7: return CTRLK_7;
	case SDLK_8: return CTRLK_8;
	case SDLK_9: return CTRLK_9;
	case SDLK_0: return CTRLK_0;
	case SDLK_MINUS: return CTRLK_MINUS;
	case SDLK_EQUALS: return CTRLK_EQUAL;
	case SDLK_q: return CTRLK_q;
	case SDLK_w: return CTRLK_w;
	case SDLK_e: return CTRLK_e;
	case SDLK_r: return CTRLK_r;
	case SDLK_t: return CTRLK_t;
	case SDLK_y: return CTRLK_y;
	case SDLK_u: return CTRLK_u;
	case SDLK_i: return CTRLK_i;
	case SDLK_o: return CTRLK_o;
	case SDLK_p: return CTRLK_p;
	case SDLK_a: return CTRLK_a;
	case SDLK_s: return CTRLK_s;
	case SDLK_d: return CTRLK_d;
	case SDLK_f: return CTRLK_f;
	case SDLK_g: return CTRLK_g;
	case SDLK_h: return CTRLK_h;
	case SDLK_j: return CTRLK_j;
	case SDLK_k: return CTRLK_k;
	case SDLK_l: return CTRLK_l;
	case SDLK_z: return CTRLK_z;
	case SDLK_x: return CTRLK_x;
	case SDLK_c: return CTRLK_c;
	case SDLK_v: return CTRLK_v;
	case SDLK_b: return CTRLK_b;
	case SDLK_n: return CTRLK_n;
	case SDLK_m: return CTRLK_m;
	case SDLK_PERIOD: return CTRLK_PERIOD;
	case SDLK_COMMA: return CTRLK_COMMA;
	case SDLK_KP_ENTER: return CTRLK_ENTER;
	}
	return CTRLK_UNKNOWN;
}

static void sdl_console_poll(struct console *up)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			if (event.key.repeat)
				break;
			__notify_controller_key_pressed(
				up->default_controller,
				sdl_to_controller_key(event.key.keysym.sym));
			break;
		case SDL_KEYUP:
			__notify_controller_key_released(
				up->default_controller,
				sdl_to_controller_key(event.key.keysym.sym));
			break;
		case SDL_TEXTINPUT:
			notify_controller_text_input(up->default_controller,
						     event.text.text[0]);
			break;
		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				unsigned short int w = event.window.data1;
				unsigned short int h = event.window.data2;
				resize_renderer(up->default_renderer, w, h);
			}
			break;
		default:
			break;
		}
	}
}

struct sdl_texture {
	struct renderer_texture up;
	struct SDL_Texture *texture;
};

static struct sdl_texture *to_sdl_texture(struct renderer_texture *up)
{
	return b6_cast_of(up, struct sdl_texture, up);
}

struct sdl_tile {
	struct renderer_tile up;
};

static struct sdl_tile *to_sdl_tile(struct renderer_tile *up)
{
	return b6_cast_of(up, struct sdl_tile, up);
}

struct sdl_base {
	struct renderer_base up;
};

static struct sdl_base *to_sdl_base(struct renderer_base *up)
{
	return b6_cast_of(up, struct sdl_base, up);
}

struct sdl_renderer {
	struct renderer up;
	struct renderer_base root;
	double x, y;
	unsigned char dim;
	struct SDL_Renderer *renderer;
};

static struct sdl_renderer *to_sdl_renderer(struct renderer *up)
{
	return b6_cast_of(up, struct sdl_renderer, up);
}

struct renderer_base *get_sdl_root(struct renderer *up)
{
	struct sdl_renderer *self = to_sdl_renderer(up);
	return &self->root;
}

static void delete_sdl_base(struct renderer_base *up)
{
	struct sdl_base *self = to_sdl_base(up);
	b6_deallocate(&b6_std_allocator, self);
}

static struct renderer_base *new_sdl_base(struct renderer *up,
					  struct renderer_base *parent,
					  const char *name, double x, double y)
{
	static const struct renderer_base_ops ops = {
		.dtor = delete_sdl_base,
	};
	struct sdl_base *self = b6_allocate(&b6_std_allocator, sizeof(*self));
	if (!self)
		return NULL;
	__setup_renderer_base(&self->up, parent, name, x, y, &ops);
	return &self->up;
}

static void delete_sdl_tile(struct renderer_tile *up)
{
	struct sdl_tile *self = to_sdl_tile(up);
	b6_deallocate(&b6_std_allocator, self);
}

struct renderer_tile *new_sdl_tile(struct renderer *up,
				   struct renderer_base *base,
				   double x, double y, double w, double h,
				   struct renderer_texture *texture)
{
	static const struct renderer_tile_ops ops = {
		.dtor = delete_sdl_tile,
	};
	struct sdl_tile *self =
		b6_allocate(&b6_std_allocator, sizeof(*self));
	if (!self)
		return NULL;
	__setup_renderer_tile(&self->up, base, x, y, w, h, texture, &ops);
	return &self->up;
}

static void delete_sdl_texture(struct renderer_texture *up)
{
	struct sdl_texture *self = to_sdl_texture(up);
	b6_deallocate(&b6_std_allocator, self);
}

static void update_sdl_texture(struct renderer_texture *up,
			       const struct rgba *rgba)
{
	struct sdl_texture *self = to_sdl_texture(up);
	SDL_UpdateTexture(self->texture, NULL, rgba->p, rgba->w * 4);
}

static struct renderer_texture *new_sdl_texture(struct renderer *up,
						const struct rgba *rgba)
{
	static const struct renderer_texture_ops ops = {
		.update = update_sdl_texture,
		.dtor = delete_sdl_texture,
	};
	struct sdl_texture *self =
		b6_allocate(&b6_std_allocator, sizeof(*self));
	if (!self)
		return NULL;
	self->up.ops = &ops;
	self->texture = SDL_CreateTexture(to_sdl_renderer(up)->renderer,
					  SDL_PIXELFORMAT_ABGR8888,
					  SDL_TEXTUREACCESS_STATIC,
					  rgba->w, rgba->h);
	b6_check(!!self->texture);
	update_sdl_texture(&self->up, rgba);
	SDL_SetTextureBlendMode(self->texture, SDL_BLENDMODE_BLEND);
	return &self->up;
}

static void sdl_render_tiles(struct sdl_renderer *self,
			     struct renderer_base *up)
{
	struct b6_dref *dref;
	for (dref = b6_list_first(&up->tiles);
	     dref != b6_list_tail(&up->tiles);
	     dref = b6_list_walk(dref, B6_NEXT)) {
		SDL_Rect dst;
		struct renderer_tile *tile =
			b6_cast_of(dref, struct renderer_tile, dref);
		struct sdl_texture *texture = to_sdl_texture(tile->texture);
		if (!texture)
			continue;
		if (!texture->texture)
			continue;
		dst.x = self->x + tile->x;
		dst.y = self->y + tile->y;
		dst.w = tile->w;
		dst.h = tile->h;
		SDL_RenderCopy(self->renderer, texture->texture, NULL, &dst);
	}
}

static void sdl_render_base(struct sdl_renderer *self, struct renderer_base *up)
{
	struct b6_dref *dref;
	double x = self->x;
	double y = self->y;
	if (!up->visible)
		return;
	self->x += up->x;
	self->y += up->y;
	sdl_render_tiles(self, up);
	for (dref = b6_list_first(&up->bases);
	     dref != b6_list_tail(&up->bases);
	     dref = b6_list_walk(dref, B6_NEXT))
		sdl_render_base(self,
				b6_cast_of(dref, struct renderer_base, dref));
	self->x = x;
	self->y = y;
}

static void sdl_render(struct renderer *up)
{
	struct sdl_renderer *self = to_sdl_renderer(up);
	SDL_RenderClear(self->renderer);
	sdl_render_base(self, &self->root);
	SDL_SetRenderDrawColor(self->renderer, 0, 0, 0, self->dim);
	SDL_SetRenderDrawBlendMode(self->renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(self->renderer, NULL);
	SDL_RenderPresent(self->renderer);
}

static void sdl_start(struct renderer *up)
{
	SDL_RenderSetLogicalSize(to_sdl_renderer(up)->renderer,
				 up->internal_width, up->internal_height);
}

static void sdl_dim(struct renderer *up, float value)
{
	struct sdl_renderer *self = to_sdl_renderer(up);
	if (value < .0f)
		self->dim = 255;
	else if (value > 1.f)
		self->dim = 0;
	else
		self->dim = (1.f - value) * 255;
}

static int open_sdl_renderer(struct sdl_renderer *self)
{
	static const struct renderer_ops ops = {
		.get_root = get_sdl_root,
		.new_base = new_sdl_base,
		.new_tile = new_sdl_tile,
		.new_texture = new_sdl_texture,
		.start = sdl_start,
		.render = sdl_render,
		.dim = sdl_dim,
	};
	int vs = get_console_vsync();
	self->x = self->y = .0;
	self->dim = 0;
	__setup_renderer(&self->up, &ops);
	__setup_renderer_base(&self->root, NULL, "sdl_root", 0, 0, NULL);
	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, sdl_accel);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, sdl_scale);
	if (vs == 0)
		SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
	else if (vs == 1)
		SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	if (!(self->renderer = SDL_CreateRenderer(window, -1, 0))) {
		log_e("%s", SDL_GetError());
		SDL_ClearHints();
		return -1;
	}
	return 0;
}

static void close_sdl_renderer(struct sdl_renderer *self)
{
	SDL_ClearHints();
	SDL_DestroyRenderer(self->renderer);
}

struct sdl_console {
	struct console up;
	struct controller controller;
	struct sdl_renderer sdl_renderer;
};

static int sdl_console_open(struct console *up)
{
	struct sdl_console *self = b6_cast_of(up, struct sdl_console, up);
	unsigned short int w = get_console_width(), h = get_console_height();
	int retval;
	if ((retval = initialize_sdl_video()))
		goto bail_out;
	if ((retval = set_sdl_video_size(w, h))) {
		finalize_sdl_video();
		goto bail_out;
	}
	if ((retval = open_sdl_renderer(&self->sdl_renderer))) {
		finalize_sdl_video();
		goto bail_out;
	}
	up->default_renderer = &self->sdl_renderer.up;
	if ((retval = get_sdl_video_size(&w, &h))) {
		log_p("could not get video size");
		goto bail_out; /* NOT REACHED */
	}
	resize_renderer(up->default_renderer, w, h);
	up->default_controller = setup_controller(&self->controller);
	SDL_StartTextInput();
bail_out:
	return retval;
}

static void sdl_console_close(struct console *up)
{
	struct sdl_console *self = b6_cast_of(up, struct sdl_console, up);
	close_sdl_renderer(&self->sdl_renderer);
	finalize_sdl_video();
	SDL_StopTextInput();
}

static void sdl_console_show(struct console *self)
{
	show_renderer(self->default_renderer);
}

static int sdl_console_register(void)
{
	static const struct console_ops ops = {
		.open = sdl_console_open,
		.poll = sdl_console_poll,
		.show = sdl_console_show,
		.close = sdl_console_close,
	};
	static struct sdl_console instance = { .up = { .ops = &ops, }, };
	return register_console(&instance.up, "sdl");
}
register_init(sdl_console_register);

struct sdl_gl_console {
	struct console up;
	struct controller controller;
	struct gl_renderer gl_renderer;
	SDL_GLContext context;
	Uint32 ticks;
};

static int sdl_gl_console_open(struct console *up)
{
	struct sdl_gl_console *self = b6_cast_of(up, struct sdl_gl_console, up);
	unsigned short int w = get_console_width(), h = get_console_height();
	int vsync = get_console_vsync();
	int retval = -1;
	if ((retval = initialize_sdl_video()))
		goto bail_out;
	if ((retval = open_gl_renderer(&self->gl_renderer)))
		goto bail_out;
	up->default_renderer = &self->gl_renderer.renderer;
	self->ticks = 0;
	flags |= SDL_WINDOW_OPENGL;
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	if ((retval = set_sdl_video_size(w, h))) {
		finalize_sdl_video();
		goto bail_out;
	}
	self->context = SDL_GL_CreateContext(window);
	if (vsync >= 0 && SDL_GL_SetSwapInterval(vsync))
		log_e("SDL_GL_SetSwapInterval(%d): %s", vsync, SDL_GetError());
	if ((retval = get_sdl_video_size(&w, &h))) {
		log_p("could not get video size");
		goto bail_out; /* NOT REACHED */
	}
	resize_renderer(up->default_renderer, w, h);
	up->default_controller = setup_controller(&self->controller);
	SDL_StartTextInput();
bail_out:
	return retval;
}

static void sdl_gl_console_close(struct console *up)
{
	struct sdl_gl_console *self = b6_cast_of(up, struct sdl_gl_console, up);
	SDL_StopTextInput();
	close_gl_renderer(&self->gl_renderer);
	SDL_GL_DeleteContext(self->context);
	finalize_sdl_video();
}

static void sdl_gl_console_show(struct console *up)
{
	struct sdl_gl_console *self = b6_cast_of(up, struct sdl_gl_console, up);
	Uint32 ticks;
	show_renderer(up->default_renderer);
	glFinish();
	SDL_GL_SwapWindow(window);
	ticks = SDL_GetTicks();
	if (ticks - self->ticks < sdl_sleep)
		SDL_Delay(sdl_sleep - (ticks - self->ticks));
	self->ticks = ticks;
}

static int sdl_gl_console_register(void)
{
	static const struct console_ops ops = {
		.open = sdl_gl_console_open,
		.poll = sdl_console_poll,
		.show = sdl_gl_console_show,
		.close = sdl_gl_console_close,
	};
	static struct sdl_gl_console instance = { .up = { .ops = &ops, }, };
	return register_console(&instance.up, "sdl/gl");
}
register_init(sdl_gl_console_register);

struct sdl_sample {
	struct sample sample;
	Mix_Chunk *chunk;
};

struct sdl_mixer {
	struct mixer mixer;
	struct sdl_sample samples[32];
	Mix_Chunk *channel[8];
	xmp_context ctx;
};

static struct sdl_mixer *to_sdl_mixer(struct mixer *mixer)
{
	return b6_cast_of(mixer, struct sdl_mixer, mixer);
}

static struct sdl_sample *to_sdl_sample(struct sample *sample)
{
	return b6_cast_of(sample, struct sdl_sample, sample);
}

static void destroy_sdl_sample(struct sample *sample)
{
	struct sdl_sample *self = to_sdl_sample(sample);
	Mix_FreeChunk(self->chunk);
	self->chunk = NULL;
}

static struct sample *sdl_mixer_import_sample(struct mixer *mixer,
					      SDL_RWops *rw_ops)
{
	const static struct sample_ops ops = { .dtor = destroy_sdl_sample, };
	struct sdl_mixer *self = to_sdl_mixer(mixer);
	int i = b6_card_of(self->samples);
	while (i--) if (!self->samples[i].chunk) {
		if (!(self->samples[i].chunk = Mix_LoadWAV_RW(rw_ops, 1)))
			break;
		self->samples[i].sample.ops = &ops;
		return &self->samples[i].sample;
	}
	return NULL;
}

static struct sample *sdl_mixer_load_sample(struct mixer *mixer,
					    const char *path)
{
	return sdl_mixer_import_sample(mixer, SDL_RWFromFile(path, "rb"));
}

static struct sample *sdl_mixer_make_sample(struct mixer *mixer,
					    const void *buf,
					    unsigned long long int len)
{
	return sdl_mixer_import_sample(mixer, SDL_RWFromConstMem(buf, len));
}

static struct sample *sdl_mixer_load_sample_from_stream(struct mixer *up,
							struct istream *is)
{
	struct sample *sample;
	struct membuf mem;
	if (initialize_membuf(&mem, &b6_std_allocator))
		return NULL;
	if (fill_membuf(&mem, is) < 0)
		return NULL;
	sample = sdl_mixer_make_sample(up, mem.buf, mem.off);
	finalize_membuf(&mem);
	return sample;
}

static void sdl_mixer_stop_sample(struct mixer *mixer, struct sample *sample)
{
	struct sdl_mixer *self = to_sdl_mixer(mixer);
	struct sdl_sample *sdl_sample = to_sdl_sample(sample);
	int i = b6_card_of(self->channel);
	while (i--)
		if (self->channel[i] == sdl_sample->chunk) {
			Mix_HaltChannel(i);
			while (self->channel[i]);
			break;
		}
}

static void sdl_mixer_play_sample(struct mixer *mixer, struct sample *sample)
{
	struct sdl_mixer *self = to_sdl_mixer(mixer);
	struct sdl_sample *sdl_sample = to_sdl_sample(sample);
	int i = b6_card_of(self->channel);
	sdl_mixer_stop_sample(mixer, sample);
	while (i--)
		if (!self->channel[i]) {
			self->channel[i] = sdl_sample->chunk;
			Mix_PlayChannel(i, sdl_sample->chunk, 0);
			break;
		}
}

static void sdl_mixer_try_play_sample(struct mixer *mixer,
				      struct sample *sample)
{
	struct sdl_mixer *self = to_sdl_mixer(mixer);
	struct sdl_sample *sdl_sample = to_sdl_sample(sample);
	int i = b6_card_of(self->channel), j = -1;
	while (i--) {
		if (!self->channel[i])
			j = i;
		if (self->channel[i] == sdl_sample->chunk)
			return;
	}
	if (j < 0)
		return;
	self->channel[j] = sdl_sample->chunk;
	Mix_PlayChannel(j, sdl_sample->chunk, 0);
}

static int sdl_mixer_load_music(struct mixer *up, const char *path)
{
	struct sdl_mixer *self = to_sdl_mixer(up);
	int retval;
	if (!(self->ctx = xmp_create_context())) {
		log_e("xmp_create_context failed");
		return -1;
	}
	retval = xmp_load_module(self->ctx, (char*)path);
	if (retval < 0)
		log_e("xmp_load_module(%s): %d\n", path, retval);
	return retval;
}

static int sdl_mixer_load_music_from_mem(struct mixer *up, const void *buf,
					 unsigned long long int len)
{
	struct sdl_mixer *self = to_sdl_mixer(up);
	int retval;
	if (!(self->ctx = xmp_create_context())) {
		log_e("xmp_create_context failed");
		return -1;
	}
	retval = xmp_load_module_from_memory(self->ctx, (void*)buf, len);
	if (retval < 0) {
		xmp_free_context(self->ctx);
		log_e("xmp_load_module_from_memory: %d\n", retval);
	}
	return retval;
}

static int sdl_mixer_load_music_from_stream(struct mixer *up,
					    struct istream *istream)
{
	struct membuf mem;
	int retval;
	if ((retval = initialize_membuf(&mem, &b6_std_allocator)))
		return retval;
	if ((retval = fill_membuf(&mem, istream)) >= 0)
		retval = sdl_mixer_load_music_from_mem(up, mem.buf, mem.off);
	finalize_membuf(&mem);
	return retval;
}

static void sdl_mixer_unload_music(struct mixer *up)
{
	struct sdl_mixer *self = to_sdl_mixer(up);
	xmp_release_module(self->ctx);
	xmp_free_context(self->ctx);
}

static void mute_hook(void *cookie, Uint8 *stream, int len)
{
	memset(stream, 0, len);
}

static void xmp_hook(void *cookie, Uint8 *stream, int len)
{
	int retval;
	if ((retval = xmp_play_buffer(cookie, stream, len, 0)))
		log_e("xmp_play_buffer: %d", retval);
}

static void sdl_mixer_play_music(struct mixer *up)
{
	struct sdl_mixer *self = to_sdl_mixer(up);
	int retval;
	if ((retval = xmp_start_player(self->ctx, 44100, 0)) < 0) {
		log_e("xmp_start_player failed: %d", retval);
		return;
	}
	SDL_LockAudio();
	Mix_HookMusic(xmp_hook, self->ctx);
	SDL_UnlockAudio();
}

static void sdl_mixer_stop_music(struct mixer *up)
{
	struct sdl_mixer *self = to_sdl_mixer(up);
	SDL_LockAudio();
	Mix_HookMusic(mute_hook, NULL);
	SDL_UnlockAudio();
	SDL_Delay(200);
	Mix_HookMusic(NULL, NULL);
	xmp_end_player(self->ctx);
}

static void sdl_mixer_set_music_pos(struct mixer *up, int pos)
{
	SDL_LockAudio();
	xmp_set_position(to_sdl_mixer(up)->ctx, pos);
	SDL_UnlockAudio();
}

static void free_sdl_audio_channel(int);

static int sdl_mixer_open(struct mixer *up)
{
	struct sdl_mixer *self = to_sdl_mixer(up);
	int i;
	if (initialize_sdl())
		return -3;
	if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		log_e("init failed", SDL_GetError());
		return -1;
	}
	for (i = b6_card_of(self->channel); i--; self->channel[i] = 0);
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096)) {
		log_e("open failed: %s.", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return -2;
	}
	Mix_ChannelFinished(free_sdl_audio_channel);
	return 0;
}

static void sdl_mixer_close(struct mixer *up)
{
	Mix_ChannelFinished(NULL);
	Mix_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	finalize_sdl();
}

static struct sdl_mixer sdl_mixer;

static int sdl_mixer_register(void)
{
	static const struct mixer_ops ops = {
		.open = sdl_mixer_open,
		.close = sdl_mixer_close,
		.load_sample = sdl_mixer_load_sample,
		.make_sample = sdl_mixer_make_sample,
		.load_sample_from_stream = sdl_mixer_load_sample_from_stream,
		.play_sample = sdl_mixer_play_sample,
		.try_play_sample = sdl_mixer_try_play_sample,
		.stop_sample = sdl_mixer_stop_sample,
		.load_music = sdl_mixer_load_music,
		.load_music_from_mem = sdl_mixer_load_music_from_mem,
		.load_music_from_stream = sdl_mixer_load_music_from_stream,
		.unload_music = sdl_mixer_unload_music,
		.play_music = sdl_mixer_play_music,
		.stop_music = sdl_mixer_stop_music,
		.set_music_pos = sdl_mixer_set_music_pos,
	};
	setup_mixer(&sdl_mixer.mixer, &ops);
	return register_mixer(&sdl_mixer.mixer, "sdl");
}
register_init(sdl_mixer_register);

static void free_sdl_audio_channel(int i)
{
	sdl_mixer.channel[i] = NULL;
}

static unsigned long long int get_sdl_clock_time(const struct b6_clock *up)
{
	return SDL_GetTicks() * 1000;
}

static void wait_sdl_clock(const struct b6_clock *up,
			   unsigned long long int delay_us)
{
	unsigned long long int delta, after, before = get_sdl_clock_time(up);
	for (;;) {
		SDL_Delay(delay_us / 1000);
		after = get_sdl_clock_time(up);
		delta = after - before;
		if (delta >= delay_us)
			break;
		delay_us -= delta;
		before = after;
	}

}

b6_ctor(register_sdl_clock_source);
void register_sdl_clock_source(void)
{
	static const struct b6_clock_ops sdl_clock_ops = {
		.get_time = get_sdl_clock_time,
		.wait = wait_sdl_clock,
	};
	static struct b6_clock sdl_clock = { .ops = &sdl_clock_ops, };
	static struct b6_named_clock sdl_named_clock = { .clock = &sdl_clock, };
	b6_register_named_clock(&sdl_named_clock, "sdl");
}
