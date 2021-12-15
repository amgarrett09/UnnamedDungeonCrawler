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
 * Dependencies: game.h, util.c
 */

typedef struct AIState {
	i32 duration; /* duration of state in turns */
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

	/* Cast a ray between entity and player */
	float opposite_length = player_pixel_y - ent_pixel_y;
	float adjacent_length = player_pixel_x - ent_pixel_x;
	float angle_tan       = -1.0f * (opposite_length / adjacent_length);

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
		entity->current_ai_state = AIST_ENEMY_CHASE;
	}
}

static void ai_enemy_chase(Entity *entity, WorldState *world_state,
			   PlayerState *player_state)
{
	printf("%s\n", "Chasing!");
}
