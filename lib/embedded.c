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

#include "embedded.h"
#include "log.h"

B6_REGISTRY_DEFINE(__embedded_registry);

int decrunch_embedded(struct embedded *self, struct ostream *ostream)
{
	unsigned char zbuf[1024];
	struct ibstream ibs;
	struct izstream izs;
	int error;
	initialize_ibstream(&ibs, self->buf, self->len);
	if (initialize_izstream(&izs, &ibs.istream, zbuf, sizeof(zbuf))) {
		log_e("cannot initialize decompressing stream");
		return -3;
	}
	for (;;) {
		unsigned char buf[1024];
		long long int rlen, wlen;
		rlen = read_istream(&izs.up, buf, sizeof(buf));
		if (!rlen) {
			error = 0;
			break;
		}
		if (rlen < 0) {
			log_w("cannot decompress stream");
			error = -2;
			break;
		}
		wlen = write_ostream(ostream, buf, rlen);
		if (rlen > wlen) {
			log_w("cannot write into stream");
			error = -2;
			break;
		}
	}
	finalize_izstream(&izs);
	return error;
}
