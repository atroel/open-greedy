/*
 * Open Greedy - an open-source version of Edromel Studio's Greedy XP
 *
 * Copyright (C) 2014-2016 Arnaud TROEL
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

#include "ghosts.h"
#include "game.h"
#include "items.h"
#include "lib/log.h"
#include "lib/rng.h"
#include <b6/heap.h>
#include "lib/std.h"

struct ghost_strategy {
	const struct ghost_strategy_ops *ops;
	struct game *game;
};

struct ghost_strategy_ops {
	void (*prepare)(struct ghost_strategy*, struct ghost*);
	float (*evaluate)(struct ghost_strategy*, struct ghost*, struct place*);
	void (*feedback)(struct ghost_strategy*, struct ghost*, enum direction);
};

static void setup_ghost_strategy(struct ghost_strategy *self,
				 const struct ghost_strategy_ops *ops,
				 struct game *game)
{
	self->ops = ops;
	self->game = game;
}

static int get_best_scores(struct ghost_strategy *self, struct ghost *ghost)
{
	float scores[4];
	enum direction d;
	struct mobile *mobile = &ghost->mobile;
	struct level *level = mobile->level;
	int i = -1;
	for_each_direction(d) {
		struct place *p = place_neighbor(level, mobile->curr, d);
		float score = p ? self->ops->evaluate(self, ghost, p) : -1;
		scores[d] = score < 0 ? -1 : 1 + score;
	}
	if (!mobile_is_stopped(mobile)) {
		d = get_mobile_direction(mobile);
		if (mobile_wants_to_go_to(mobile, d)) {
			d = get_opposite(d);
			if (scores[d] > 0)
			       scores[d] = 0;
		}
	}
	for_each_direction(d)
		if (scores[d] >= 0 && (i < 0 || scores[d] > scores[i]))
			i = d;
	return i;
}

static struct ghost_strategy fallback_ghost_strategy;

static void run_ghost_strategy(struct ghost_strategy *self, struct ghost *ghost)
{
	struct mobile *mobile = &ghost->mobile;
	int i;
	if (self->ops->prepare)
		self->ops->prepare(self, ghost);
	i = get_best_scores(self, ghost);
	if (i < 0 && get_ghost_state(ghost) != GHOST_ZOMBIE)
		i = get_best_scores(&fallback_ghost_strategy, ghost);
	if (self->ops->feedback)
		self->ops->feedback(self, ghost, i);
	cancel_all_mobile_moves(mobile);
	if (i >= 0)
		submit_mobile_move(mobile, i);
}

struct janitor_strategy {
	struct ghost_strategy strategy;
	float count[LEVEL_WIDTH][LEVEL_HEIGHT];
};

static float evaluate_janitor(struct ghost_strategy *strategy,
			      struct ghost *ghost, struct place *place)
{
	struct janitor_strategy *self =
		b6_cast_of(strategy, struct janitor_strategy, strategy);
	struct mobile *mobile = &ghost->mobile;
	int x, y;
	place_location(mobile->level, place, &x, &y);
	return 1 / self->count[x][y];
}

static void feedback_janitor(struct ghost_strategy *strategy,
			     struct ghost *ghost, enum direction d)
{
	struct janitor_strategy *self =
		b6_cast_of(strategy, struct janitor_strategy, strategy);
	struct mobile *mobile = &ghost->mobile;
	struct level *level = mobile->level;
	int x, y;
	place_location(level, place_neighbor(level, mobile->curr, d), &x, &y);
	self->count[x][y] += 1;
}

static void initialize_janitor_strategy(struct janitor_strategy *self,
					struct game *game)
{
	static const struct ghost_strategy_ops ops = {
		.evaluate = evaluate_janitor,
		.feedback = feedback_janitor,
	};
	int x, y;
	for (y = 0; y < LEVEL_HEIGHT; y++) for (x = 0; x < LEVEL_WIDTH; x++)
		self->count[x][y] = 1;
	setup_ghost_strategy(&self->strategy, &ops, game);
}

struct astar_node {
	unsigned short int f;
	unsigned short int g;
	unsigned short int r;
};

static unsigned int get_manhattan_distance(int x1, int y1, int x2, int y2)
{
	return (x2 >= x1 ? x2 - x1 : x1 - x2) + (y2 >= y1 ? y2 - y1 : y1 - y2);
}

static void reset_astar_node(struct astar_node *n)
{
	n->r = 0xffff;
}

static int astar_node_is_open(const struct astar_node *n)
{
	return n->r < 0xfffe;
}

static void open_astar_node(struct astar_node *node, unsigned short int g,
			    unsigned short int f)
{
	node->g = g;
	node->f = g + f;
}

static void close_astar_node(struct astar_node *n)
{
	n->r = 0xfffe;
}

static int astar_node_is_closed(const struct astar_node *n)
{
	return n->r == 0xfffe;
}

static int astar_node_compare(void *lptr, void *rptr)
{
	const struct astar_node *lhs = lptr;
	const struct astar_node *rhs = rptr;
	return b6_sign_of((int)lhs->f - (int)rhs->f);
}

static void astar_node_set_index(void *ptr, unsigned long int r)
{
	((struct astar_node*)ptr)->r = r;
}

static struct place *node_to_place(struct astar_node* node,
				   struct astar_node *nodes,
				   struct level *level)
{
	return &level->places[node - nodes];
}

static struct astar_node *coords_to_node(int x, int y, struct astar_node *nodes)
{
	return &nodes[x + y * LEVEL_WIDTH];
}

static struct astar_node *place_to_node(struct place *place,
					struct astar_node *nodes,
					struct level *level)
{
	int x, y;
	place_location(level, place, &x, &y);
	return coords_to_node(x, y, nodes);
}

static unsigned int get_shortest_distance(const struct level *level,
					  int xs, int ys, int xd, int yd)
{
	unsigned int d = get_manhattan_distance(xs, ys, xd, yd);
	if (level->teleport_places[0]) {
		int x[2], y[2];
		unsigned int dt[2];
		place_location(level, level->teleport_places[0], &x[0], &y[0]);
		place_location(level, level->teleport_places[1], &x[1], &y[1]);
		dt[0] = get_manhattan_distance(xs, ys, x[0], y[0]) +
			get_manhattan_distance(x[1], y[1], xd, yd);
		dt[1] = get_manhattan_distance(xs, ys, x[1], y[1]) +
			get_manhattan_distance(x[0], y[0], xd, yd);
		if (d > dt[0])
			d = dt[0];
		if (d > dt[1])
			d = dt[1];
	}
	return d;
}

static int get_astar_distance(struct level *level, const struct items *items,
			      unsigned short int xs, unsigned short int ys,
			      unsigned short int xd, unsigned short int yd)
{
	int retval = -1;
	struct astar_node nodes[LEVEL_WIDTH * LEVEL_HEIGHT];
	struct astar_node *goal, *curr;
	struct b6_array array;
	struct b6_heap queue;
	struct level_iterator i;
	b6_array_initialize(&array, &b6_std_allocator,
			    sizeof(struct astar_node*));
	b6_heap_reset(&queue, &array, astar_node_compare, astar_node_set_index);
	initialize_level_iterator(&i, level);
	while (level_iterator_has_next(&i))
		reset_astar_node(place_to_node(level_iterator_next(&i), nodes,
					       level));
	goal = coords_to_node(xd, yd, nodes);
	curr = coords_to_node(xs, ys, nodes);
	open_astar_node(curr, 0, get_shortest_distance(level, xs, ys, xd, yd));
	b6_heap_push(&queue, curr);
	do {
		struct place *place;
		unsigned short int f, g;
		enum direction d;
		curr = b6_heap_top(&queue);
		if (curr == goal) {
			retval = goal->f;
			break;
		}
		b6_heap_pop(&queue);
		close_astar_node(curr);
		place = node_to_place(curr, nodes, level);
		g = 1 + curr->g;
		for_each_direction(d) {
			struct place *n = place_neighbor(level, place, d);
			struct astar_node *node;
			int x, y;
			if (!n)
				continue;
			if (is_teleport(items, n->item))
				n = get_teleport_destination(n->item);
			place_location(level, n, &x, &y);
			node = coords_to_node(x, y, nodes);
			if (astar_node_is_closed(node) && g >= node->g)
				continue;
			if (!astar_node_is_open(node)) {
				open_astar_node(node, g, get_shortest_distance(
						level, x, y, xd, yd));
				b6_heap_push(&queue, node);
				continue;
			}
			if (g >= node->g)
				continue;
			f = g + get_shortest_distance(level, x, y, xd, yd);
			if (node->f == f) {
				node->g = g;
				continue;
			}
			if (node->f < f) {
				logf_w("%u < %u", node->f, f);
				continue;
			}
			node->g = g;
			node->f = f;
			b6_heap_touch(&queue, node->r);
		}
	} while (!b6_heap_empty(&queue));
	b6_array_finalize(&array);
	return retval;
}

static float get_no_score(struct ghost_strategy *self, struct ghost *ghost,
			  struct place *place)
{
	return -1;
}

static float get_fallback_score(struct ghost_strategy *self,
				struct ghost *ghost, struct place *place)
{
	return read_random_number_generator();
}

static float get_doggy_score(struct ghost_strategy *self, struct ghost *ghost,
			     struct place *place)
{
	int xd = self->game->pacman.mobile.x;
	int yd = self->game->pacman.mobile.y;
	int xs, ys;
	float d;
	place_location(ghost->mobile.level, place, &xs, &ys);
	d = get_shortest_distance(ghost->mobile.level, xs, ys, xd, yd);
	return 1 / (1 + d);
}

static float get_astar_score(struct ghost_strategy *self, struct ghost *ghost,
			     struct place *place)
{
	struct level *level = ghost->mobile.level;
	int xs, ys, xd, yd;
	float d;
	place_location(level, place, &xs, &ys);
	place_location(level, self->game->pacman.mobile.curr, &xd, &yd);
	d = get_astar_distance(level, self->game->level.items, xs, ys, xd, yd);
	return d < 0 ? d : 1 - (1 + d) / LEVEL_WIDTH / LEVEL_HEIGHT;
}

static float get_zombie_score(struct ghost_strategy *self, struct ghost *ghost,
			      struct place *place)
{
	struct level *level = ghost->mobile.level;
	int xs, ys, xd, yd;
	float d;
	if (ghost->mobile.curr == level->ghosts_home) {
		cancel_all_mobile_moves(&ghost->mobile);
		return -1;
	}
	place_location(level, place, &xs, &ys);
	place_location(level, level->ghosts_home, &xd, &yd);
	d = get_astar_distance(level, self->game->level.items, xs, ys, xd, yd);
	return d < 0 ? d : 1 - (1 + d) / LEVEL_WIDTH / LEVEL_HEIGHT;
}

struct rogue_strategy {
	struct ghost_strategy up;
	struct ghost_strategy none;
	struct ghost_strategy doggy;
	struct ghost_strategy astar;
	struct ghost_strategy *strategy;
};

static void setup_no_strategy(struct ghost_strategy *self, struct game *game)
{
	static const struct ghost_strategy_ops ops = {
		.evaluate = get_no_score,
	};
	setup_ghost_strategy(self, &ops, game);
}

static void setup_doggy_strategy(struct ghost_strategy *self, struct game *game)
{
	static const struct ghost_strategy_ops ops = {
		.evaluate = get_doggy_score,
	};
	setup_ghost_strategy(self, &ops, game);
}

static void setup_astar_strategy(struct ghost_strategy *self, struct game *game)
{
	static const struct ghost_strategy_ops ops = {
		.evaluate = get_astar_score,
	};
	setup_ghost_strategy(self, &ops, game);
}

static void prepare_rogue(struct ghost_strategy *up, struct ghost *ghost)
{
	struct rogue_strategy *self = b6_cast_of(up, struct rogue_strategy, up);
	switch ((int)(read_random_number_generator() * 3)) {
	case 0: self->strategy = &self->none; break;
	case 1: self->strategy = &self->doggy; break;
	default: self->strategy = &self->astar;
	}
	if (self->strategy->ops->prepare)
		self->strategy->ops->prepare(self->strategy, ghost);
}

static float get_rogue_score(struct ghost_strategy *up, struct ghost *ghost,
			     struct place *place)
{
	struct rogue_strategy *self = b6_cast_of(up, struct rogue_strategy, up);
	b6_check(self->strategy);
	return self->strategy->ops->evaluate(self->strategy, ghost, place);
}

static void setup_rogue_strategy(struct rogue_strategy *self, struct game *game)
{
	static const struct ghost_strategy_ops ops = {
		.prepare = prepare_rogue,
		.evaluate = get_rogue_score,
	};
	setup_no_strategy(&self->none, game);
	setup_doggy_strategy(&self->doggy, game);
	setup_astar_strategy(&self->astar, game);
	setup_ghost_strategy(&self->up, &ops, game);
}

static float get_afraid_score(struct ghost_strategy *up, struct ghost *ghost,
			      struct place *place)
{
	float f = get_rogue_score(up, ghost, place);
	return f < 0 ? f : 1 - f;
}

static void setup_afraid_strategy(struct rogue_strategy *self,
				  struct game *game)
{
	static const struct ghost_strategy_ops ops = {
		.prepare = prepare_rogue,
		.evaluate = get_afraid_score,
	};
	setup_no_strategy(&self->none, game);
	setup_doggy_strategy(&self->doggy, game);
	setup_astar_strategy(&self->astar, game);
	setup_ghost_strategy(&self->up, &ops, game);
}

static struct ghost_strategy no_ghost_strategy;
static struct janitor_strategy janitor_strategy;
static struct ghost_strategy doggy_ghost_strategy;
static struct ghost_strategy astar_ghost_strategy;
static struct ghost_strategy zombie_ghost_strategy;
static struct rogue_strategy afraid_ghost_strategy;
static struct rogue_strategy rogue_ghost_strategy;

void initialize_ghost_strategies(struct game *game)
{
	static const struct ghost_strategy_ops zombie_ops = {
		.evaluate = get_zombie_score,
	};
	static const struct ghost_strategy_ops fallback_ops = {
		.evaluate = get_fallback_score,
	};
	setup_ghost_strategy(&zombie_ghost_strategy, &zombie_ops, game);
	initialize_janitor_strategy(&janitor_strategy, game);
	setup_ghost_strategy(&fallback_ghost_strategy, &fallback_ops, game);
	setup_no_strategy(&no_ghost_strategy, game);
	setup_doggy_strategy(&doggy_ghost_strategy, game);
	setup_astar_strategy(&astar_ghost_strategy, game);
	setup_rogue_strategy(&rogue_ghost_strategy, game);
	setup_afraid_strategy(&afraid_ghost_strategy, game);
}

static void ghost_enter(struct mobile *mobile)
{
	struct ghost *self = b6_cast_of(mobile, struct ghost, mobile);
	ghost_visits_item(mobile, mobile->curr->item);
	run_ghost_strategy(self->current_strategy, self);
}

void initialize_ghost(struct ghost *self, int n, float speed,
		      const struct b6_clock *clock)
{
	static const struct mobile_ops ops = { .enter = ghost_enter, };
	initialize_mobile(&self->mobile, &ops, clock, speed);
	switch (n) {
	case 0:
		self->default_strategy = &doggy_ghost_strategy;
		break;
	case 1:
		self->default_strategy = &rogue_ghost_strategy.up;
		break;
	case 2:
		self->default_strategy = &rogue_ghost_strategy.up;
		break;
	default:
		self->default_strategy = &astar_ghost_strategy;
		break;
	}
}

static void ghost_turns_around(struct ghost *self)
{
	struct mobile *mobile = &self->mobile;
	enum direction d = get_opposite(mobile->direction);
	cancel_mobile_move(mobile, d);
	submit_mobile_move(mobile, d);
}

void set_ghost_state(struct ghost *self, enum ghost_state state)
{
	struct mobile *mobile = &self->mobile;
	unlock_mobile(mobile);
	switch (state) {
	case GHOST_OUT:
		break;
	case GHOST_FROZEN:
		lock_mobile(mobile);
		break;
	case GHOST_HUNTER:
		self->current_strategy = self->default_strategy;
		if (self->state == GHOST_AFRAID)
			ghost_turns_around(self);
		else if (self->state == GHOST_OUT)
			ghost_enter(mobile);
		break;
	case GHOST_AFRAID:
		self->current_strategy = &afraid_ghost_strategy.up;
		if (self->state == GHOST_HUNTER)
			ghost_turns_around(self);
		break;
	case GHOST_ZOMBIE:
		cancel_all_mobile_moves(mobile);
		self->current_strategy = &zombie_ghost_strategy;
		break;
	}
	self->state = state;
}
