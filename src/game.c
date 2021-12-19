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
 * Dependendencies: <string.h>, game.h, , util.c, memory.c tile_map.c,
 * hashmap.c
 */

static const i32 SCREEN_HEIGHT_PIXELS = SCREEN_HEIGHT_TILES * TILE_HEIGHT;
static const i32 SCREEN_WIDTH_PIXELS  = SCREEN_WIDTH_TILES * TILE_WIDTH;

static void check_and_prep_screen_transition(WorldState *world_state,
					     PlayerState *player_state,
					     MapSegment *map_segments);
static void display_bitmap_tile(u32 *image_buffer, Bitmap *bmp, i32 tile_number,
				i32 target_x, i32 target_y, i32 tile_width,
				i32 tile_height, bool mirrored);

static size_t load_bitmap(const char file_path[], void *load_location,
			  size_t max_size);
static void move_player(WorldState *world_state, PlayerState *player_state,
			ScreenState *screen_state);
static void move_entities(Entities *entities, ScreenState *screen_state);
static void handle_player_collision(WorldState *world_state,
				    PlayerState *player_state, Input *input,
				    ScreenState *screen_state);
static void hot_tile_push(ScreenState *screen_state, u32 tile_x, u32 tile_y);
static void render_entities(u32 *image_buffer, MapSegment *map_segment);
static void render_hot_tiles(ScreenState *screen_state,
			     WorldState *world_state);
static void render_player(u32 *image_buffer, PlayerState *player_state);
static void render_rectangle(u32 *image_buffer, i32 min_x, i32 max_x, i32 min_y,
			     i32 max_y, float red, float green, float blue);
static void render_status_bar(u32 *image_buffer);
static void render_map_segment(u32 *image_buffer, MapSegment *map_segment,
			       void *tile_set, i32 x_offset, i32 y_offset);
static void scroll_screens(u32 *image_buffer, PlayerState *player_state,
			   WorldState *world_state);
static void warp_to_screen(u32 *image_buffer, PlayerState *player_state,
			   WorldState *world_state);

void game_initialize_memory(Memory *memory, ScreenState *screen_state, i32 dt)
{
	PlayerState *player_state = &memory->player_state;
	WorldState *world_state   = &memory->world_state;
	load_bitmap("resources/player_sprites.bmp",
		    (void *)player_state->player_sprites,
		    MAX_PLAYER_SPRITE_SIZE);

	world_state->tile_set = mem_load_file_to_temp_storage(
		memory, "resources/tile_set.bmp", &load_bitmap, false);

	world_state->tile_props =
		hash_create_hash_int(memory, mem_reserve_temp_storage);

	i32 tile_map_rc =
		tm_load_tile_map("resources/maps/test_tilemap.tm", memory);

	if (tile_map_rc == 0) {
		world_state->current_map_segment = &memory->map_segments[0];
		Vec2 test_entity_position        = {.x = 10, .y = 5};

		Entities *entities =
			&world_state->current_map_segment->entities;
		entities->data[entities->num_entities].position =
			test_entity_position;
		entities->data[entities->num_entities].id =
			entities->num_entities + 1;
		entities->data[entities->num_entities++].face_direction =
			RIGHTDIR;

		u32 key = util_compactify_three_u32(
			0, (u32)test_entity_position.x,
			(u32)test_entity_position.y);
		u64 value = (u64)1 << 32;

		hash_insert_int(&world_state->tile_props, key, value);
	}

	player_state->tile_x = 15;
	player_state->tile_y = 3;
	player_state->pixel_x =
		util_convert_tile_to_pixel(player_state->tile_x, X_DIMENSION);
	player_state->pixel_y =
		util_convert_tile_to_pixel(player_state->tile_y, Y_DIMENSION);
	player_state->move_counter   = 0;
	player_state->move_direction = NULLDIR;
	player_state->sprite_number  = 2;

	world_state->turn_duration = 8 * (16 / dt);

	render_map_segment(screen_state->image_buffer,
			   world_state->current_map_segment,
			   world_state->tile_set, 0, 0);
}

void game_update_and_render(Memory *memory, Input *input,
			    ScreenState *screen_state)
{
	PlayerState *player_state = &memory->player_state;
	WorldState *world_state   = &memory->world_state;
	u32 *image_buffer         = screen_state->image_buffer;

	if (world_state->trans_state == TRANS_STATE_SCROLLING) {
		scroll_screens(image_buffer, player_state, world_state);
		return;
	}

	if (world_state->trans_state == TRANS_STATE_WARPING) {
		warp_to_screen(image_buffer, player_state, world_state);
		return;
	}

	if (player_state->move_counter <= 0) {
		check_and_prep_screen_transition(world_state, player_state,
						 memory->map_segments);

		if (world_state->trans_state != TRANS_STATE_NORMAL &&
		    world_state->trans_state != TRANS_STATE_WAITING)
			return;

		handle_player_collision(world_state, player_state, input,
					screen_state);

	} else {
		Entities *entities =
			&world_state->current_map_segment->entities;
		ai_run_ai_system(entities, world_state, player_state);
		move_entities(entities, screen_state);
		move_player(world_state, player_state, screen_state);
	}

	render_hot_tiles(screen_state, world_state);

	render_entities(image_buffer, world_state->current_map_segment);

	render_player(image_buffer, player_state);

	render_status_bar(image_buffer);
}

static void check_and_prep_screen_transition(WorldState *world_state,
					     PlayerState *player_state,
					     MapSegment *map_segments)
{
	MapSegment *old_map_segment = world_state->current_map_segment;
	i32 tile_x                  = player_state->tile_x;
	i32 tile_y                  = player_state->tile_y;
	i32 segment_index           = old_map_segment->index;

	if (tile_y == -1 && old_map_segment->top_connection) {
		world_state->trans_state          = TRANS_STATE_SCROLLING;
		world_state->transition_counter   = SCREEN_HEIGHT_PIXELS;
		world_state->transition_direction = UPDIR;
		world_state->next_map_segment = old_map_segment->top_connection;
	} else if (tile_y == SCREEN_HEIGHT_TILES &&
		   old_map_segment->bottom_connection) {
		world_state->trans_state          = TRANS_STATE_SCROLLING;
		world_state->transition_counter   = SCREEN_HEIGHT_PIXELS;
		world_state->transition_direction = DOWNDIR;
		world_state->next_map_segment =
			old_map_segment->bottom_connection;
	} else if (tile_x == -1 && old_map_segment->left_connection) {
		world_state->trans_state          = TRANS_STATE_SCROLLING;
		world_state->transition_counter   = SCREEN_WIDTH_PIXELS;
		world_state->transition_direction = LEFTDIR;
		world_state->next_map_segment =
			old_map_segment->left_connection;
	} else if (tile_x == SCREEN_WIDTH_TILES &&
		   old_map_segment->right_connection) {
		world_state->trans_state          = TRANS_STATE_SCROLLING;
		world_state->transition_counter   = SCREEN_WIDTH_PIXELS;
		world_state->transition_direction = RIGHTDIR;
		world_state->next_map_segment =
			old_map_segment->right_connection;
	} else {
		i32 tile_x             = player_state->tile_x;
		i32 tile_y             = player_state->tile_y;
		IntHashMap *tile_props = &world_state->tile_props;
		u32 key = util_compactify_three_u32((u32)segment_index,
						    (u32)tile_x, (u32)tile_y);
		u64 current_tile_props = hash_get_int(tile_props, key);

		bool is_warp_tile = !!(current_tile_props & TPROP_IS_WARP_TILE);
		u32 warp_map      = (current_tile_props & TPROP_WARP_MAP) >>
			TPROP_WARP_MAP_SHIFT;

		if (is_warp_tile && warp_map < MAX_MAP_SEGMENTS &&
		    world_state->trans_state != TRANS_STATE_WAITING) {

			world_state->trans_state      = TRANS_STATE_WARPING;
			world_state->next_map_segment = &map_segments[warp_map];
		}
	}
}

static void display_bitmap_tile(u32 *restrict image_buffer,
				Bitmap *restrict bmp, i32 tile_number,
				i32 target_x, i32 target_y, i32 tile_width,
				i32 tile_height, bool mirrored)
{
	if (!tile_number)
		return;

	i32 bmp_tile_number = tile_number - 1;
	i32 bmp_width       = bmp->width;
	i32 bmp_height      = bmp->height;

	/* Calculate starting x and y position in bmp based on tile number */
	i32 source_x = (bmp_tile_number * tile_width) % bmp_width;
	i32 source_y =
		(bmp_tile_number / (bmp_width / tile_width)) * tile_height;

	u32 *image = (u32 *)bmp->data;

	/* BMP pixels are arranged bottom to top so start at bottom */
	i32 bmp_row_start = (bmp_height - 1) * bmp_width;
	bmp_row_start -= source_y * bmp_width;

	/*
	 * Make sure following loop never goes out of bounds
	 * (also clips tile at screen edge)
	 */
	i32 start_row    = 0;
	i32 end_row      = tile_height;
	i32 start_column = 0;
	i32 end_column   = tile_width;

	if (target_y >= WIN_HEIGHT - tile_height) {
		end_row -= target_y - (WIN_HEIGHT - tile_height);
	} else if (target_y < 0) {
		start_row -= target_y;
		bmp_row_start += target_y * bmp_width;
	}

	if (target_x >= WIN_WIDTH - tile_width) {
		end_column -= target_x - (WIN_WIDTH - tile_width);
	} else if (target_x < 0) {
		start_column -= target_x;
	}

	for (i32 row = start_row; row < end_row; row++) {
		for (i32 column = start_column; column < end_column; column++) {
			i32 source_column =
				mirrored ? end_column - column - 1 : column;

			i32 target_row = row + target_y;

			i32 target_column = column + target_x;

			i32 source_pixel =
				bmp_row_start + source_column + source_x;
			i32 target_pixel =
				target_row * WIN_WIDTH + target_column;

			u32 bmp_color    = image[source_pixel];
			u32 buffer_color = image_buffer[target_pixel];

			/* Linear Alpha Blend bmp with existing data in buffer*/
			u32 alpha     = (bmp_color & 0xff000000) >> 24;
			float opacity = alpha / 255.0f;

			u32 bmp_red   = (bmp_color & 0x00ff0000) >> 16;
			u32 bmp_green = (bmp_color & 0x0000ff00) >> 8;
			u32 bmp_blue  = (bmp_color & 0x000000ff);

			u32 buffer_red   = (buffer_color & 0x00ff0000) >> 16;
			u32 buffer_green = (buffer_color & 0x0000ff00) >> 8;
			u32 buffer_blue  = (buffer_color & 0x000000ff);

			float new_red = ((1 - opacity) * buffer_red) +
				(opacity * bmp_red);
			float new_green = ((1 - opacity) * buffer_green) +
				(opacity * bmp_green);
			float new_blue = ((1 - opacity) * buffer_blue) +
				(opacity * bmp_blue);

			u32 new_color = ((u32)new_red << 16) |
				((u32)new_green << 8) | (u32)new_blue;

			image_buffer[target_pixel] = new_color;
		}

		bmp_row_start -= bmp_width;
	}
}

static void handle_player_collision(WorldState *world_state,
				    PlayerState *player_state, Input *input,
				    ScreenState *screen_state)
{
	i32 segment_index      = world_state->current_map_segment->index;
	IntHashMap *tile_props = &world_state->tile_props;

	u32 keys = input->keys;

	if (keys & KEYMASK_UP || keys & KEYMASK_DOWN) {
		bool is_up     = !(keys & KEYMASK_DOWN);
		i32 change     = is_up ? -1 : 1;
		i32 new_tile_y = player_state->tile_y + change;
		i32 tile_x     = player_state->tile_x;

		player_state->sprite_number  = is_up ? 6 : 2;
		player_state->move_direction = is_up ? UPDIR : DOWNDIR;

		bool is_not_colliding = true;
		u32 key = util_compactify_three_u32((u32)segment_index & 0xFFFF,
						    (u32)tile_x & 0xFF,
						    (u32)new_tile_y & 0xFF);
		u64 current_tile_props = hash_get_int(tile_props, key);

		if (new_tile_y >= 0 && new_tile_y < SCREEN_HEIGHT_TILES &&
		    tile_x >= 0 && tile_x < SCREEN_WIDTH_TILES) {
			is_not_colliding =
				!(current_tile_props & TPROP_HAS_COLLISION) &&
				!(current_tile_props & TPROP_ENTITY);
		}

		if (is_not_colliding) {
			player_state->tile_y       = new_tile_y;
			player_state->move_counter = TILE_HEIGHT;
		} else {
			hot_tile_push(screen_state, (u32)player_state->tile_x,
				      (u32)player_state->tile_y);
		}

		world_state->trans_state = TRANS_STATE_NORMAL;
	} else if (keys & KEYMASK_RIGHT || keys & KEYMASK_LEFT) {
		bool is_right  = !(keys & KEYMASK_LEFT);
		i32 change     = is_right ? 1 : -1;
		i32 new_tile_x = player_state->tile_x + change;
		i32 tile_y     = player_state->tile_y;

		player_state->sprite_number  = 10;
		player_state->move_direction = is_right ? RIGHTDIR : LEFTDIR;

		bool is_not_colliding = true;
		u32 key = util_compactify_three_u32((u32)segment_index & 0xFFFF,
						    (u32)new_tile_x & 0xFF,
						    (u32)tile_y & 0xFF);
		u64 current_tile_props = hash_get_int(tile_props, key);

		if (new_tile_x >= 0 && new_tile_x < SCREEN_WIDTH_TILES &&
		    tile_y >= 0 && tile_y < SCREEN_HEIGHT_TILES) {
			is_not_colliding =
				!(current_tile_props & TPROP_HAS_COLLISION) &&
				!(current_tile_props & TPROP_ENTITY);
		}

		if (is_not_colliding) {
			player_state->tile_x       = new_tile_x;
			player_state->move_counter = TILE_WIDTH;
		} else {
			hot_tile_push(screen_state, (u32)player_state->tile_x,
				      (u32)player_state->tile_y);
		}

		world_state->trans_state = TRANS_STATE_NORMAL;
	}
}

static void hot_tile_push(ScreenState *screen_state, u32 tile_x, u32 tile_y)
{
	if (tile_x < 0 || tile_y < 0 || tile_x >= SCREEN_WIDTH_TILES ||
	    tile_y >= SCREEN_HEIGHT_TILES)
		return;

	u32 value = ((tile_x & 0xFFFF) << 16) | (tile_y & 0xFFFF);

	screen_state->hot_tiles[screen_state->hot_tiles_length++] = value;
}

static size_t load_bitmap(const char file_path[], void *load_location,
			  size_t max_size)
{
	size_t result = debug_platform_load_asset(
		file_path, (void *)load_location, max_size);

	if (!result) {
		return 0;
	}

	BMPHeader *header = (BMPHeader *)load_location;

	u32 image_offset = header->image_offset;
	i32 image_width  = header->image_width;
	i32 image_height = header->image_height;

	u32 red_mask   = header->red_mask;
	u32 green_mask = header->green_mask;
	u32 blue_mask  = header->blue_mask;
	u32 alpha_mask = header->alpha_mask;

	/* Discard header and copy image data to location */
	unsigned char *image_data = ((unsigned char *)header) + image_offset;
	Bitmap *bmp               = (Bitmap *)load_location;
	bmp->width                = image_width;
	bmp->height               = image_height;

	memcpy(bmp->data, image_data, (size_t)(image_width * image_height * 4));

	/*
	 * For swizzling:
	 * How many bits we need to shift to get values to start at bit 0
	 */
	u32 red_shift   = (u32)util_bit_scan_forward_u(red_mask);
	u32 green_shift = (u32)util_bit_scan_forward_u(green_mask);
	u32 blue_shift  = (u32)util_bit_scan_forward_u(blue_mask);
	u32 alpha_shift = (u32)util_bit_scan_forward_u(alpha_mask);

	red_shift   = red_shift < 0 ? 0 : red_shift;
	green_shift = green_shift < 0 ? 0 : green_shift;
	blue_shift  = blue_shift < 0 ? 0 : blue_shift;
	alpha_shift = alpha_shift < 0 ? 0 : alpha_shift;

	u32 *image = (u32 *)bmp->data;

	for (i32 i = 0; i < image_width * image_height; i++) {
		u32 color = image[i];

		/* Swizzle color values to ensure ARGB order */
		u32 alpha = (((color & alpha_mask) >> alpha_shift) & 0xFF)
			<< 24;
		u32 red   = (((color & red_mask) >> red_shift) & 0xFF) << 16;
		u32 green = (((color & green_mask) >> green_shift) & 0xFF) << 8;
		u32 blue  = (((color & blue_mask) >> blue_shift) & 0xFF);

		image[i] = alpha | red | green | blue;
	}

	size_t bitmap_size =
		(size_t)(image_width * image_height * 4) + sizeof(Bitmap);

	return bitmap_size;
}

static void move_player(WorldState *world_state, PlayerState *player_state,
			ScreenState *screen_state)
{
	switch (player_state->move_direction) {
	case UPDIR:
		hot_tile_push(screen_state, (u32)player_state->tile_x,
			      (u32)(player_state->tile_y + 1));
		hot_tile_push(screen_state, (u32)player_state->tile_x,
			      (u32)player_state->tile_y);
		player_state->pixel_y -=
			TILE_HEIGHT / world_state->turn_duration;
		break;
	case RIGHTDIR:
		hot_tile_push(screen_state, (u32)(player_state->tile_x - 1),
			      (u32)player_state->tile_y);
		hot_tile_push(screen_state, (u32)player_state->tile_x,
			      (u32)player_state->tile_y);
		player_state->pixel_x +=
			TILE_WIDTH / world_state->turn_duration;
		break;
	case DOWNDIR:
		hot_tile_push(screen_state, (u32)player_state->tile_x,
			      (u32)(player_state->tile_y - 1));
		hot_tile_push(screen_state, (u32)player_state->tile_x,
			      (u32)player_state->tile_y);
		player_state->pixel_y +=
			TILE_HEIGHT / world_state->turn_duration;
		break;
	case LEFTDIR:
		hot_tile_push(screen_state, (u32)(player_state->tile_x + 1),
			      (u32)player_state->tile_y);
		hot_tile_push(screen_state, (u32)player_state->tile_x,
			      (u32)player_state->tile_y);
		player_state->pixel_x -=
			TILE_WIDTH / world_state->turn_duration;
		break;
	default:
		break;
	}

	player_state->move_counter -= TILE_WIDTH / world_state->turn_duration;
}

/*
 * TODO: need a better method of determining which tiles are hot / make
 * sure we don't push the same tiles to the hot list more than once
 * */
static void move_entities(Entities *entities, ScreenState *screen_state)
{
	for (i32 i = 0; i < entities->num_entities; i++) {
		Entity *ent = &entities->data[i];
		if (ent->current_ai_state == AIST_ENEMY_CHASE) {
			hot_tile_push(screen_state, (u32)ent->position.x,
				      (u32)(ent->position.y - 1));
			hot_tile_push(screen_state, (u32)ent->position.x,
				      (u32)(ent->position.y + 1));
			hot_tile_push(screen_state, (u32)(ent->position.x - 1),
				      (u32)ent->position.y);
			hot_tile_push(screen_state, (u32)(ent->position.x + 1),
				      (u32)ent->position.y);
		}
	}
}

static void render_entities(u32 *image_buffer, MapSegment *map_segment)
{
	Entities *entities = &map_segment->entities;
	i32 num_entities   = entities->num_entities;

	for (i32 i = 0; i < num_entities; i++) {
		i32 tile_x  = entities->data[i].position.x;
		i32 tile_y  = entities->data[i].position.y;
		i32 pixel_x = util_convert_tile_to_pixel(tile_x, X_DIMENSION);
		i32 pixel_y = util_convert_tile_to_pixel(tile_y, Y_DIMENSION);

		render_rectangle(image_buffer, pixel_x - 16, pixel_x + 16,
				 pixel_y - 16, pixel_y + 16, 0.0f, 1.0f, 1.0f);
	}
}

static void render_hot_tiles(ScreenState *screen_state, WorldState *world_state)
{
	u32 *tiles           = (u32 *)world_state->current_map_segment->tiles;
	u32 *hot_tiles       = screen_state->hot_tiles;
	i32 hot_tiles_length = screen_state->hot_tiles_length;
	u32 *image_buffer    = screen_state->image_buffer;

	if (!hot_tiles_length)
		return;

	for (i32 i = 0; i < hot_tiles_length; i++) {
		u32 data   = hot_tiles[i];
		u32 tile_x = (data & HT_TILE_X) >> HT_TILE_X_SHIFT;
		u32 tile_y = data & HT_TILE_Y;

		u32 tile_data = tiles[tile_y * SCREEN_WIDTH_TILES + tile_x];
		u32 target_y  = tile_y * TILE_HEIGHT;
		u32 target_x  = tile_x * TILE_WIDTH;

		u32 bg_tile_number =
			(tile_data & TM_BG_TILE) >> TM_BG_TILE_SHIFT;
		u32 fg_tile_number = tile_data & TM_FG_TILE;

		display_bitmap_tile(image_buffer, world_state->tile_set,
				    (i32)bg_tile_number, (i32)target_x,
				    (i32)target_y, TILE_WIDTH, TILE_HEIGHT,
				    false);
		if (fg_tile_number) {
			display_bitmap_tile(image_buffer, world_state->tile_set,
					    (i32)fg_tile_number, (i32)target_x,
					    (i32)target_y, TILE_WIDTH,
					    TILE_HEIGHT, false);
		}
	}

	screen_state->hot_tiles_length = 0;
}

static void render_player(u32 *image_buffer, PlayerState *player_state)
{
	i32 player_min_x = player_state->pixel_x - 16;
	i32 player_min_y = player_state->pixel_y - 16;

	i32 sprite_number = player_state->sprite_number;
	bool mirrored     = player_state->move_direction == LEFTDIR;

	display_bitmap_tile(image_buffer, (void *)player_state->player_sprites,
			    sprite_number, player_min_x, player_min_y,
			    TILE_WIDTH, TILE_HEIGHT, mirrored);
}

static void render_rectangle(u32 *image_buffer, i32 min_x, i32 max_x, i32 min_y,
			     i32 max_y, float red, float green, float blue)
{
	i32 clamped_min_x = min_x >= 0 ? min_x : 0;
	i32 clamped_min_y = min_y >= 0 ? min_y : 0;
	i32 clamped_max_x = max_x > WIN_WIDTH ? WIN_WIDTH : max_x;
	i32 clamped_max_y = max_y > WIN_HEIGHT ? WIN_HEIGHT : max_y;

	u32 int_red   = (u32)(red * 255.0f);
	u32 int_green = (u32)(green * 255.0f);
	u32 int_blue  = (u32)(blue * 255.0f);

	u32 color = (int_red << 16) | (int_green << 8) | int_blue;

	for (int y = clamped_min_y; y < clamped_max_y; y++) {
		for (int x = clamped_min_x; x < clamped_max_x; x++) {
			image_buffer[y * WIN_WIDTH + x] = color;
		}
	}
}

static void render_status_bar(u32 *image_buffer)
{
	render_rectangle(image_buffer, 0, 1280, 640, 720, 1.0f, 0.0f, 1.0f);
}

static void render_map_segment(u32 *image_buffer, MapSegment *map_segment,
			       void *tile_set, i32 x_offset, i32 y_offset)
{
	if (!tile_set | !map_segment | !image_buffer)
		return;

	u32 *tiles = (u32 *)map_segment->tiles;
	for (i32 row = 0; row < SCREEN_HEIGHT_TILES; row++) {
		for (i32 column = 0; column < SCREEN_WIDTH_TILES; column++) {
			i32 target_y = row * TILE_HEIGHT;
			i32 target_x = column * TILE_WIDTH;
			u32 tile_data =
				tiles[row * SCREEN_WIDTH_TILES + column];
			u32 bg_tile_number =
				(tile_data & TM_BG_TILE) >> TM_BG_TILE_SHIFT;
			u32 fg_tile_number = tile_data & TM_FG_TILE;

			display_bitmap_tile(image_buffer, tile_set,
					    (i32)bg_tile_number,
					    (i32)target_x + x_offset,
					    (i32)target_y + y_offset,
					    TILE_WIDTH, TILE_HEIGHT, false);

			display_bitmap_tile(image_buffer, tile_set,
					    (i32)fg_tile_number,
					    (i32)target_x + x_offset,
					    (i32)target_y + y_offset,
					    TILE_WIDTH, TILE_HEIGHT, false);
		}
	}
}

/*
 * TODO: May want to implement scrolling in the platform layer and then draw
 * in only part of the screen each frame, instead of redrawing all of it
 */
static void scroll_screens(u32 *image_buffer, PlayerState *player_state,
			   WorldState *world_state)
{
	Direction transition_direction = world_state->transition_direction;

	i32 transition_speed_y = 16;
	i32 transition_speed_x = 20;

	/* If we're done transitioning... */
	if (world_state->transition_counter <= 0) {
		world_state->trans_state = TRANS_STATE_NORMAL;
		world_state->current_map_segment =
			world_state->next_map_segment;

		/* Put player in correct spot on new map */
		switch (transition_direction) {
		case UPDIR:
			player_state->tile_y += SCREEN_HEIGHT_TILES;
			player_state->pixel_y = util_convert_tile_to_pixel(
				player_state->tile_y, Y_DIMENSION);
			break;
		case DOWNDIR:
			player_state->tile_y -= SCREEN_HEIGHT_TILES;
			player_state->pixel_y = util_convert_tile_to_pixel(
				player_state->tile_y, Y_DIMENSION);
			break;
		case RIGHTDIR:
			player_state->tile_x -= SCREEN_WIDTH_TILES;
			player_state->pixel_x = util_convert_tile_to_pixel(
				player_state->tile_x, X_DIMENSION);
			break;
		case LEFTDIR:
			player_state->tile_x += SCREEN_WIDTH_TILES;
			player_state->pixel_x = util_convert_tile_to_pixel(
				player_state->tile_x, X_DIMENSION);
			break;
		default:
			break;
		}
		return;
	}

	if (transition_direction == UPDIR || transition_direction == DOWNDIR) {
		world_state->transition_counter -= transition_speed_y;
	} else {
		world_state->transition_counter -= transition_speed_x;
	}

	/*
	 * Old tile map moves off one side of screen, new tile map comes in
	 * from other side. Counter ticks down each frame. Map offsets are
	 * based on counter.
	 */
	i32 old_map_y_offset = 0;
	i32 old_map_x_offset = 0;
	i32 new_map_y_offset = 0;
	i32 new_map_x_offset = 0;
	switch (transition_direction) {
	case UPDIR:
		old_map_y_offset =
			SCREEN_HEIGHT_PIXELS - world_state->transition_counter;
		new_map_y_offset = -world_state->transition_counter;
		player_state->pixel_y += transition_speed_y;
		break;
	case DOWNDIR:
		old_map_y_offset =
			world_state->transition_counter - SCREEN_HEIGHT_PIXELS;
		new_map_y_offset = world_state->transition_counter;
		player_state->pixel_y -= transition_speed_y;
		break;
	case RIGHTDIR:
		old_map_x_offset =
			world_state->transition_counter - SCREEN_WIDTH_PIXELS;
		new_map_x_offset = world_state->transition_counter;
		player_state->pixel_x -= transition_speed_x;
		break;
	case LEFTDIR:
		old_map_x_offset =
			SCREEN_WIDTH_PIXELS - world_state->transition_counter;
		new_map_x_offset = -world_state->transition_counter;
		player_state->pixel_x += transition_speed_x;
		break;
	default:
		break;
	}

	render_map_segment(image_buffer, world_state->next_map_segment,
			   world_state->tile_set, new_map_x_offset,
			   new_map_y_offset);

	render_map_segment(image_buffer, world_state->current_map_segment,
			   world_state->tile_set, old_map_x_offset,
			   old_map_y_offset);

	render_player(image_buffer, player_state);

	render_status_bar(image_buffer);
}

static void warp_to_screen(u32 *image_buffer, PlayerState *player_state,
			   WorldState *world_state)
{
	i32 segment_index      = world_state->current_map_segment->index;
	i32 tile_x             = player_state->tile_x;
	i32 tile_y             = player_state->tile_y;
	IntHashMap *tile_props = &world_state->tile_props;
	u32 key                = util_compactify_three_u32(
                (u32)segment_index, (u32)tile_x & 0xFF, (u32)tile_y & 0xFF);

	u64 current_tile_props = hash_get_int(tile_props, key);
	player_state->tile_x =
		(current_tile_props & TPROP_WTILE_X) >> TPROP_WTILE_X_SHIFT;
	player_state->tile_y =
		(current_tile_props & TPROP_WTILE_Y) >> TPROP_WTILE_Y_SHIFT;
	player_state->pixel_y =
		util_convert_tile_to_pixel(player_state->tile_y, Y_DIMENSION);
	player_state->pixel_x =
		util_convert_tile_to_pixel(player_state->tile_x, X_DIMENSION);

	render_map_segment(image_buffer, world_state->next_map_segment,
			   world_state->tile_set, 0, 0);

	render_player(image_buffer, player_state);

	render_status_bar(image_buffer);

	world_state->current_map_segment = world_state->next_map_segment;
	world_state->next_map_segment    = NULL;
	world_state->trans_state         = TRANS_STATE_WAITING;
}
