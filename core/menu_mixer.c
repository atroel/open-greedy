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

#include "menu_mixer.h"
#include "mixer.h"
#include "data.h"
#include "lib/log.h"

int initialize_menu_mixer(struct menu_mixer *self, struct menu *menu,
			  const char *skin, struct mixer *mixer)
{
	static struct menu_observer_ops ops = {};
	struct istream *is;
	struct data_entry *data;
	self->menu = menu;
	self->mixer = mixer;
	self->music = 0;
	if ((data = lookup_data(skin, audio_data_type, "menu.music")) &&
	    (is = get_data(data))) {
		int retval = mixer->ops->load_music_from_stream(mixer, is);
		if (!retval) {
			play_music(mixer);
			self->music = 1;
		} else
			log_w("could not load background music");
		put_data(data, is);
	} else
		log_w("cannot find background music");
	add_menu_observer(self->menu,
			  setup_menu_observer(&self->menu_observer, &ops));
	return 0;
}

void finalize_menu_mixer(struct menu_mixer *self)
{
	del_menu_observer(&self->menu_observer);
	if (self->music) {
		stop_music(self->mixer);
		unload_music(self->mixer);
	}
}
