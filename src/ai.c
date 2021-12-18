/*
 * Copyright (C) 2021 Alex Garrett
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Dependencies: game.h, util.c, hashmap.c
 */

#define MAX_ASTAR_NODES 128

typedef struct AIState {
	i32 duration; /* in turns */
	void (*do_action)(Entity *, WorldState *, PlayerState *);
	AIStateIndex next_state;
} AIState;

static void ai_enemy_idle(Entity *, WorldState *world_state,
			  PlayerState *player_state);
static void ai_enemy_chase(Entity *entity, WorldState *world_state,
			   PlayerState *player_state);

static const AIState ai_states[] = {
	{1, ai_enemy_idle, AIST_ENEMY_IDLE},  /* AIST_ENEMY_IDLE */
	{1, ai_enemy_chase, AIST_ENEMY_CHASE} /* AIST_ENEMY_CHASE */
};

void ai_run_ai_system(Entities *entities, WorldState *world_state,
		      PlayerState *player_state)
{
	i32 num_entities = entities->num_entities;
	Entity *ent_data = entities->data;
	for (i32 i = 0; i < num_entities; i++) {
		Entity *ent   = &ent_data[i];
		AIState state = ai_states[ent->current_ai_state];

		if (ent->ai_counter == 0) {
			ent->current_ai_state = state.next_state;
			ent->ai_counter =
				state.duration * world_state->turn_duration - 1;
			state.do_action(ent, world_state, player_state);
		} else {
			ent->ai_counter--;
		}
	}
}

static void ai_enemy_idle(Entity *entity, WorldState *world_state,
			  PlayerState *player_state)
{
	i32 segment_index        = world_state->current_map_segment->index;
	IntHashMap *tile_props   = &world_state->tile_props;
	Direction face_direction = entity->face_direction;

	float player_pixel_x = (float)util_convert_tile_to_pixel(
		player_state->tile_x, X_DIMENSION);
	float player_pixel_y = (float)util_convert_tile_to_pixel(
		player_state->tile_y, Y_DIMENSION);
	float ent_pixel_x = (float)util_convert_tile_to_pixel(
		entity->position.x, X_DIMENSION);
	float ent_pixel_y = (float)util_convert_tile_to_pixel(
		entity->position.y, Y_DIMENSION);

	bool player_is_above = player_pixel_y < ent_pixel_y;
	bool player_is_below = player_pixel_y > ent_pixel_y;
	bool player_is_right = player_pixel_x > ent_pixel_x;
	bool player_is_left  = player_pixel_x < ent_pixel_x;

	switch (face_direction) {
	case UPDIR: {
		if (player_is_below)
			return;
		break;
	}
	case RIGHTDIR: {
		if (player_is_left)
			return;
		break;
	}
	case DOWNDIR: {
		if (player_is_above)
			return;
		break;
	}
	case LEFTDIR: {
		if (player_is_right)
			return;
		break;
	}
	default:
		break;
	}

	bool player_visible = true;

	/* Handle special cases where there's no angle between ent and player */
	if (!player_is_above && !player_is_below) {
		i32 ent_tile_x = entity->position.x;
		i32 ent_tile_y = entity->position.y;
		i32 loop_end   = player_is_right
			? player_state->tile_x - ent_tile_x
			: ent_tile_x - player_state->tile_x;

		for (i32 i = 1; i < loop_end; i++) {
			i32 test_tile_x = player_is_right ? ent_tile_x + i
							  : ent_tile_x - i;

			u32 key = util_compactify_three_u32(
				segment_index, test_tile_x, ent_tile_y);
			u64 props = hash_get_int(tile_props, key);
			if (props & TPROP_HAS_COLLISION ||
			    props & TPROP_ENTITY) {
				player_visible = false;
				break;
			}
		}

		goto raycast_end;
	}

	if (!player_is_right && !player_is_left) {
		i32 ent_tile_x = entity->position.x;
		i32 ent_tile_y = entity->position.y;
		i32 loop_end   = player_is_below
			? player_state->tile_y - ent_tile_y
			: ent_tile_y - player_state->tile_y;

		for (i32 i = 1; i < loop_end; i++) {
			i32 test_tile_y = player_is_below ? ent_tile_y + i
							  : ent_tile_y - i;

			u32 key = util_compactify_three_u32(
				segment_index, ent_tile_x, test_tile_y);
			u64 props = hash_get_int(tile_props, key);

			if (props & TPROP_HAS_COLLISION ||
			    props & TPROP_ENTITY) {
				player_visible = false;
				break;
			}
		}

		goto raycast_end;
	}

	/* Cast a ray between entity and player */
	float opposite_length = player_pixel_y - ent_pixel_y;
	float adjacent_length = player_pixel_x - ent_pixel_x;
	float angle_tan       = -1.0f * (opposite_length / adjacent_length);

	/* Check horizontal intersections */
	i32 y_offset = player_is_above ? -TILE_HEIGHT : TILE_HEIGHT;
	i32 x_offset = (i32)((float)TILE_HEIGHT / angle_tan);

	i32 test_point_y;
	if (player_is_above) {
		test_point_y =
			(i32)(ent_pixel_y / (float)TILE_HEIGHT) * TILE_HEIGHT -
			1;
	} else {
		x_offset *= -1;
		test_point_y =
			(i32)(ent_pixel_y / (float)TILE_HEIGHT) * TILE_HEIGHT +
			TILE_HEIGHT;
	}

	i32 test_point_x =
		ent_pixel_x + (ent_pixel_y - (float)test_point_y) / angle_tan;

	i32 test_tile_x = test_point_x / TILE_WIDTH;
	i32 test_tile_y = test_point_y / TILE_HEIGHT;

	bool should_loop = true;
	while (should_loop) {
		u32 key = util_compactify_three_u32(segment_index, test_tile_x,
						    test_tile_y);
		u64 props = hash_get_int(tile_props, key);
		if (props & TPROP_HAS_COLLISION || props & TPROP_ENTITY) {
			player_visible = false;
			break;
		}

		test_point_x += x_offset;
		test_point_y += y_offset;
		test_tile_x = test_point_x / TILE_WIDTH;
		test_tile_y = test_point_y / TILE_HEIGHT;

		switch (face_direction) {
		case UPDIR: {
			should_loop = test_tile_y > player_state->tile_y;
			break;
		}
		case RIGHTDIR: {
			should_loop = test_tile_x < player_state->tile_x;
			break;
		}
		case DOWNDIR: {
			should_loop = test_tile_y < player_state->tile_y;
			break;
		}
		case LEFTDIR: {
			should_loop = test_tile_x > player_state->tile_x;
			break;
		}
		default:
			should_loop = false;
			break;
		}
	}

	if (!player_visible)
		goto raycast_end;

	/* Check vertical intersections */
	x_offset = player_is_right ? TILE_WIDTH : -TILE_WIDTH;
	y_offset = (i32)((float)TILE_HEIGHT * angle_tan);

	if (player_is_right) {
		y_offset *= -1;
		test_point_x =
			(i32)(ent_pixel_x / (float)TILE_WIDTH) * TILE_WIDTH +
			TILE_HEIGHT;
	} else {
		test_point_x =
			(i32)(ent_pixel_x / (float)TILE_WIDTH) * TILE_WIDTH - 1;
	}

	test_point_y =
		ent_pixel_y + (ent_pixel_x - (float)test_point_x) * angle_tan;

	test_tile_x = test_point_x / TILE_WIDTH;
	test_tile_y = test_point_y / TILE_HEIGHT;

	should_loop = true;
	while (should_loop) {
		u32 key = util_compactify_three_u32(segment_index, test_tile_x,
						    test_tile_y);
		u64 props = hash_get_int(tile_props, key);
		if (props & TPROP_HAS_COLLISION || props & TPROP_ENTITY) {
			player_visible = false;
			break;
		}

		test_point_x += x_offset;
		test_point_y += y_offset;
		test_tile_x = test_point_x / TILE_WIDTH;
		test_tile_y = test_point_y / TILE_HEIGHT;

		switch (face_direction) {
		case UPDIR: {
			should_loop = test_tile_y > player_state->tile_y;
			break;
		}
		case RIGHTDIR: {
			should_loop = test_tile_x < player_state->tile_x;
			break;
		}
		case DOWNDIR: {
			should_loop = test_tile_y < player_state->tile_y;
			break;
		}
		case LEFTDIR: {
			should_loop = test_tile_x > player_state->tile_x;
			break;
		}
		default:
			should_loop = false;
			break;
		}
	}

raycast_end:
	if (player_visible) {
		printf("%s\n", "player visible!");
		if (player_is_below && face_direction != DOWNDIR) {
			entity->face_direction = DOWNDIR;
		} else if (player_is_above && face_direction != UPDIR) {
			entity->face_direction = UPDIR;
		}

		entity->current_ai_state = AIST_ENEMY_CHASE;
	}
}

static i32 astar_compute_fcost(AStarNode node)
{
	return node.g_cost + node.h_cost;
}

static i32 astar_compute_distance(Vec2 pos1, Vec2 pos2)
{
	i32 x = util_abs(pos1.x - pos2.x);
	i32 y = util_abs(pos1.y - pos2.y);

	i32 out = x >= y ? 14 * y + 10 * (x - y) : 14 * x + 10 * (y - x);

	return out;
}

static i32 astar_delete_open_node(AStarNode open_nodes[MAX_ASTAR_NODES],
				  i32 node_index, i32 open_nodes_length)
{
	if (open_nodes_length == 0)
		return 0;
	i32 last_index         = open_nodes_length - 1;
	AStarNode temp         = open_nodes[last_index];
	open_nodes[last_index] = open_nodes[node_index];
	open_nodes[node_index] = temp;

	return last_index;
}

static i32 astar_find_open_node(AStarNode open_nodes[MAX_ASTAR_NODES],
				Vec2 position, i32 open_nodes_length)
{
	i32 index = -1;
	for (int i = 0; i < open_nodes_length; i++) {
		AStarNode node = open_nodes[i];

		if (node.position.x == position.x &&
		    node.position.y == position.y) {
			index = i;
			break;
		}
	}

	return index;
}

static void ai_enemy_chase(Entity *entity, WorldState *world_state,
			   PlayerState *player_state)
{
	/* TODO: update enemy collision */
	if (entity->path_counter) {
		i32 index = MAX_PATH_LENGTH - entity->path_counter;
		if (index < entity->path_cache.length) {
			entity->position = entity->path_cache.data[index];
		}
		entity->path_counter--;
		return;
	}
	Vec2 ent_pos    = entity->position;
	Vec2 player_pos = {.x = player_state->tile_x,
			   .y = player_state->tile_y};

	AStarNode open_nodes[MAX_ASTAR_NODES] = {0};
	AStarHashMap closed_nodes             = {.data = {{0}}};
	i32 open_nodes_length                 = 0;

	open_nodes[open_nodes_length].position.x = ent_pos.x;
	open_nodes[open_nodes_length].position.y = ent_pos.y;
	open_nodes[open_nodes_length].parent.x   = -1;
	open_nodes[open_nodes_length].parent.y   = -1;
	open_nodes[open_nodes_length].g_cost     = 0;
	open_nodes[open_nodes_length++].h_cost =
		astar_compute_distance(player_pos, ent_pos);

	AStarNode current_node = open_nodes[0];
	i32 current_index      = 0;

	while (open_nodes_length > 0 && open_nodes_length < MAX_ASTAR_NODES) {
		current_node  = open_nodes[0];
		current_index = 0;
		/*
		 * Find node with lowest fcost
		 * NOTE: If this is slow, we can change to a min-heap
		 */
		for (i32 i = 0; i < open_nodes_length; i++) {
			i32 probing_node_fcost =
				astar_compute_fcost(open_nodes[i]);
			i32 current_node_fcost =
				astar_compute_fcost(current_node);

			if (probing_node_fcost < current_node_fcost ||
			    (probing_node_fcost == current_node_fcost &&
			     open_nodes[i].h_cost < current_node.h_cost)) {
				current_node  = open_nodes[i];
				current_index = i;
			}
		}

		/* Remove current node from open and add to closed */
		u32 key = util_compactify_two_u32(current_node.position.x,
						  current_node.position.y);
		hash_insert_astar(&closed_nodes, key, current_node);
		open_nodes_length = astar_delete_open_node(
			open_nodes, current_index, open_nodes_length);

		/* If this is our target, we're done */
		if (current_node.position.x == player_pos.x &&
		    current_node.position.y == player_pos.y) {
			break;
		}

		Vec2 neighbors[4] = {
			{current_node.position.x, current_node.position.y - 1},
			{current_node.position.x + 1, current_node.position.y},
			{current_node.position.x, current_node.position.y + 1},
			{current_node.position.x - 1, current_node.position.y},
		};

		for (i32 i = 0; i < 4; i++) {
			Vec2 neighbor = neighbors[i];
			i32 map_segment_index =
				world_state->current_map_segment->index;
			u32 tprop_key = util_compactify_three_u32(
				map_segment_index, neighbor.x, neighbor.y);
			u64 tile_props = hash_get_int(&world_state->tile_props,
						      tprop_key);
			/* If neighbor not traversable, skip */
			if ((tile_props & TPROP_HAS_COLLISION) ||
			    (tile_props & TPROP_ENTITY)) {
				continue;
			}

			u32 closed_key =
				util_compactify_two_u32(neighbor.x, neighbor.y);
			AStarNode closed_node =
				hash_get_astar(&closed_nodes, closed_key);

			/* If neighbor already closed, skip */
			if (closed_node.position.x > -1) {
				continue;
			}

			i32 new_cost_to_neighbor = current_node.g_cost +
				astar_compute_distance(current_node.position,
						       neighbor);
			i32 neighbor_index = astar_find_open_node(
				open_nodes, neighbor, open_nodes_length);

			if (neighbor_index < 0) {
				/*
				 * Add neighbor to open nodes and set
				 * parent to current node
				 */
				AStarNode new_node = {
					.position = {neighbor.x, neighbor.y},
					.parent   = {current_node.position.x,
                                                   current_node.position.y},
					.g_cost   = new_cost_to_neighbor,
					.h_cost   = astar_compute_distance(
                                                neighbor, player_pos)};

				open_nodes[open_nodes_length++] = new_node;
			} else if (new_cost_to_neighbor <
				   open_nodes[neighbor_index].g_cost) {
				/*
				 * Update parent and g and h costs of neighbor
				 * if already in open nodes
				 */
				open_nodes[neighbor_index].parent.x =
					current_node.position.x;
				open_nodes[neighbor_index].parent.y =
					current_node.position.y;
				open_nodes[neighbor_index].g_cost =
					new_cost_to_neighbor;
				open_nodes[neighbor_index].h_cost =
					astar_compute_distance(neighbor,
							       player_pos);
			}
		}
	}

	/* Retrace path to start */
	Vec2 path_positions[MAX_ASTAR_NODES] = {0};
	i32 path_position_length             = 0;

	path_positions[path_position_length++] = current_node.position;

	Vec2 parent = current_node.parent;
	while (path_position_length < MAX_ASTAR_NODES) {
		if (parent.x == ent_pos.x && parent.y == ent_pos.y) {
			break;
		}

		u32 key = util_compactify_two_u32(parent.x, parent.y);
		AStarNode parent_node = hash_get_astar(&closed_nodes, key);

		if (parent_node.position.x < 0)
			break;

		path_positions[path_position_length++] = parent_node.position;
		parent                                 = parent_node.parent;
	}

	/* Store first several node positions with entity */
	i32 j = 0;
	for (i32 i = path_position_length - 1; i >= 0; i--) {
		if (j >= MAX_PATH_LENGTH)
			break;

		entity->path_cache.data[j] = path_positions[i];
		j++;
	}

	entity->path_cache.length = j;
	entity->path_counter      = MAX_PATH_LENGTH;
	entity->position =
		entity->path_cache.data[MAX_PATH_LENGTH - entity->path_counter];
	entity->path_counter--;
}
