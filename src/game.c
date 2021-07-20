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
 * Dependendencies: <string.h>, game.h, tile_maps.c
 */

static const i32 SCREEN_HEIGHT_PIXELS = SCREEN_HEIGHT_TILES * TILE_HEIGHT;
static const i32 SCREEN_WIDTH_PIXELS  = SCREEN_WIDTH_TILES * TILE_WIDTH;

static i32 bit_scan_forward_u(u32 number);
static i32 convert_tile_to_pixel(i32 tile_value, CoordDimension dimension);
static void display_bitmap_tile(i32 *image_buffer, Bitmap *bmp, i32 tile_number,
				i32 target_x, i32 target_y, i32 tile_width,
				i32 tile_height, bool mirrored);

static size_t get_next_aligned_offset(size_t start_offset, size_t min_to_add,
				      size_t alignment);
static void *load_bitmap(const char file_path[], Memory *memory);
static size_t load_tile_map(const char file_path[], Memory *memory);
static void move_player(PlayerState *player_state);
static bool check_and_prep_screen_transition(WorldState *world_state,
					     PlayerState *player_state);
static void handle_player_collision(WorldState *world_state,
				    PlayerState *player_state, Input *input);
static void play_sound(Sound *game_sound);
static void render_player(i32 *image_buffer, PlayerState *player_state);
static void render_rectangle(i32 *image_buffer, i32 min_x, i32 max_x, i32 min_y,
			     i32 max_y, float red, float green, float blue);
static void render_status_bar(i32 *image_buffer);
static void render_tile_map(i32 *image_buffer, TileMap *tile_map,
			    void *tile_set, i32 x_offset, i32 y_offset);
static void set_tile_map_value(i32 x, i32 y, i32 layer, i32 tile_number,
			       TileMap *tile_map);
static void transition_screens(i32 *image_buffer, PlayerState *player_state,
			       WorldState *world_state);

void game_initialize_memory(Memory *memory, i32 dt)
{
	PlayerState *player_state = &memory->player_state;
	WorldState *world_state   = &memory->world_state;

	void *sprite_load_result =
		load_bitmap("resources/player_sprites.bmp", memory);
	if (sprite_load_result) {
		player_state->sprite_location = sprite_load_result;
	}

	void *tile_load_result = load_bitmap("resources/tile_set.bmp", memory);
	if (tile_load_result) {
		world_state->tile_set = tile_load_result;
	}

	size_t tile_map_load_result =
		load_tile_map("resources/maps/tilemap.tm", memory);

	if (tile_map_load_result) {
		world_state->current_tile_map = &memory->tile_maps[0];
	}

	player_state->tile_x = 15;
	player_state->tile_y = 3;
	player_state->pixel_x =
		convert_tile_to_pixel(player_state->tile_x, X_DIMENSION);
	player_state->pixel_y =
		convert_tile_to_pixel(player_state->tile_y, Y_DIMENSION);
	player_state->move_counter   = 0;
	player_state->move_direction = NULLDIR;
	player_state->speed          = ((TILE_WIDTH / 8) * dt) / 16;
	player_state->sprite_number  = 2;
}

void game_update_and_render(Memory *memory, Input *input, Sound *game_sound,
			    i32 *image_buffer)
{
	PlayerState *player_state = &memory->player_state;
	WorldState *world_state   = &memory->world_state;

	play_sound(game_sound);

	if (world_state->screen_transitioning) {
		transition_screens(image_buffer, player_state, world_state);
		return;
	}

	if (player_state->move_counter <= 0) {
		if (check_and_prep_screen_transition(world_state,
						     player_state)) {
			return;
		}

		handle_player_collision(world_state, player_state, input);
	} else {
		move_player(player_state);
	}

	render_tile_map(image_buffer, world_state->current_tile_map,
			world_state->tile_set, 0, 0);

	render_player(image_buffer, player_state);

	render_status_bar(image_buffer);
}

/*
 * Finds first set bit in an unsigned integer, starting from lowest bit.
 * Returns the index of the set bit.
 */
static i32 bit_scan_forward_u(u32 number)
{
	/* TODO: Use intrinsic for this */
	bool found = false;
	i32 index  = 0;

	while (!found) {
		if (index > 31) {
			index = -1;
			break;
		} else if (number & (1 << index)) {
			found = true;
		} else {
			index++;
		}
	}

	return index;
}

static i32 convert_tile_to_pixel(i32 tile_value, CoordDimension dimension)
{
	if (dimension == Y_DIMENSION) {
		return TILE_HEIGHT * tile_value + (TILE_HEIGHT / 2);
	} else {
		return TILE_WIDTH * tile_value + (TILE_WIDTH / 2);
	}
}

static void display_bitmap_tile(i32 *restrict image_buffer,
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

	i32 *image = (i32 *)bmp->data;

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

			i32 bmp_color    = image[source_pixel];
			i32 buffer_color = image_buffer[target_pixel];

			/* Linear Alpha Blend bmp with existing data in buffer*/
			i32 alpha     = (bmp_color & 0xff000000) >> 24;
			float opacity = alpha / 255.0f;

			i32 bmp_red   = (bmp_color & 0x00ff0000) >> 16;
			i32 bmp_green = (bmp_color & 0x0000ff00) >> 8;
			i32 bmp_blue  = (bmp_color & 0x000000ff);

			i32 buffer_red   = (buffer_color & 0x00ff0000) >> 16;
			i32 buffer_green = (buffer_color & 0x0000ff00) >> 8;
			i32 buffer_blue  = (buffer_color & 0x000000ff);

			float new_red = ((1 - opacity) * buffer_red) +
				(opacity * bmp_red);
			float new_green = ((1 - opacity) * buffer_green) +
				(opacity * bmp_green);
			float new_blue = ((1 - opacity) * buffer_blue) +
				(opacity * bmp_blue);

			i32 new_color = ((i32)new_red << 16) |
				((i32)new_green << 8) | (i32)new_blue;

			image_buffer[target_pixel] = new_color;
		}

		bmp_row_start -= bmp_width;
	}
}

static size_t get_next_aligned_offset(size_t start_offset, size_t min_to_add,
				      size_t alignment)
{
	size_t min_new_offset = start_offset + min_to_add;
	size_t remainder      = min_new_offset % alignment;

	if (remainder) {
		return min_new_offset + (alignment - remainder);
	} else {
		return min_new_offset;
	}
}

static void handle_player_collision(WorldState *world_state,
				    PlayerState *player_state, Input *input)
{
	i32 *current_collision_map =
		(i32 *)world_state->current_tile_map->collision_map;
	i32 keys = input->keys;

	if (keys & KEYMASK_UP || keys & KEYMASK_DOWN) {
		bool is_up     = !(keys & KEYMASK_DOWN);
		i32 change     = is_up ? -1 : 1;
		i32 new_tile_y = player_state->tile_y + change;
		i32 tile_x     = player_state->tile_x;

		player_state->sprite_number  = is_up ? 6 : 2;
		player_state->move_direction = is_up ? UPDIR : DOWNDIR;

		bool is_not_colliding = true;

		if (new_tile_y >= 0 && new_tile_y < SCREEN_HEIGHT_TILES &&
		    tile_x >= 0 && tile_x < SCREEN_WIDTH_TILES) {
			is_not_colliding =
				!current_collision_map
					[new_tile_y * SCREEN_WIDTH_TILES +
					 tile_x];
		}

		if (is_not_colliding) {
			player_state->tile_y       = new_tile_y;
			player_state->move_counter = TILE_HEIGHT;
		}
	} else if (keys & KEYMASK_RIGHT || keys & KEYMASK_LEFT) {
		bool is_right  = !(keys & KEYMASK_LEFT);
		i32 change     = is_right ? 1 : -1;
		i32 new_tile_x = player_state->tile_x + change;
		i32 tile_y     = player_state->tile_y;

		player_state->sprite_number  = 10;
		player_state->move_direction = is_right ? RIGHTDIR : LEFTDIR;

		bool is_not_colliding = true;

		if (new_tile_x >= 0 && new_tile_x < SCREEN_WIDTH_TILES &&
		    tile_y >= 0 && tile_y < SCREEN_HEIGHT_TILES) {
			is_not_colliding =
				!current_collision_map
					[tile_y * SCREEN_WIDTH_TILES +
					 new_tile_x];
		}

		if (is_not_colliding) {
			player_state->tile_x       = new_tile_x;
			player_state->move_counter = TILE_WIDTH;
		}
	}
}

// TODO: maybe pass an expected size parameter and fail if file is larger
static void *load_bitmap(const char file_path[], Memory *memory)
{
	char *load_location =
		(char *)memory->temp_storage + memory->temp_next_load_offset;

	size_t result =
		debug_platform_load_asset(file_path, (void *)load_location);

	if (!result) {
		return NULL;
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
	char *image_data = ((char *)header) + image_offset;
	Bitmap *bmp      = (Bitmap *)load_location;
	bmp->width       = image_width;
	bmp->height      = image_height;

	memcpy(bmp->data, image_data, image_width * image_height * 4);

	/*
	 * For swizzling:
	 * How many bits we need to shift to get values to start at bit 0
	 */
	i32 red_shift   = bit_scan_forward_u(red_mask);
	i32 green_shift = bit_scan_forward_u(green_mask);
	i32 blue_shift  = bit_scan_forward_u(blue_mask);
	i32 alpha_shift = bit_scan_forward_u(alpha_mask);

	red_shift   = red_shift < 0 ? 0 : red_shift;
	green_shift = green_shift < 0 ? 0 : green_shift;
	blue_shift  = blue_shift < 0 ? 0 : blue_shift;
	alpha_shift = alpha_shift < 0 ? 0 : alpha_shift;

	i32 *image = (i32 *)bmp->data;

	for (i32 i = 0; i < image_width * image_height; i++) {
		i32 color = image[i];

		/* Swizzle color values to ensure ARGB order */
		i32 alpha = (((color & alpha_mask) >> alpha_shift) & 0xFF)
			<< 24;
		i32 red   = (((color & red_mask) >> red_shift) & 0xFF) << 16;
		i32 green = (((color & green_mask) >> green_shift) & 0xFF) << 8;
		i32 blue  = (((color & blue_mask) >> blue_shift) & 0xFF);

		image[i] = alpha | red | green | blue;
	}

	size_t bitmap_size = image_width * image_height * 4 + sizeof(Bitmap);
	memory->temp_next_load_offset = get_next_aligned_offset(
		memory->temp_next_load_offset, bitmap_size, 32);

	return (void *)load_location;
}

static size_t load_tile_map(const char file_path[], Memory *memory)
{
	char *temp_location =
		(char *)memory->temp_storage + memory->temp_next_load_offset;

	size_t result =
		debug_platform_load_asset(file_path, (void *)temp_location);

	if (!result)
		return 0;

	TileMap *tile_map = &memory->tile_maps[0];

	i32 tile_map_index = 0;
	for (i32 y = 0; y < SCREEN_HEIGHT_TILES; y++) {
		for (i32 x = 0; x < SCREEN_WIDTH_TILES; x++) {
			i32 tile_number = 0;
			i32 layer       = 0;
			for (i32 j = 0; j < 128; j++) {
				char c = temp_location[tile_map_index++];

				if (c == '\n' || c == 0) {
					break;
				} else if (c == ',') {
					if (tile_number > 0) {
						set_tile_map_value(x, y, layer,
								   tile_number,
								   tile_map);
					}
					tile_number = 0;
					layer += 1;
				} else if (c >= 48 && c <= 57) {
					tile_number =
						tile_number * 10 + (c - '0');
				} else {
					continue;
				}
			}
		}
	}

	return result;
}

static void move_player(PlayerState *player_state)
{
	switch (player_state->move_direction) {
	case UPDIR:
		player_state->pixel_y -= player_state->speed;
		break;
	case RIGHTDIR:
		player_state->pixel_x += player_state->speed;
		break;
	case DOWNDIR:
		player_state->pixel_y += player_state->speed;
		break;
	case LEFTDIR:
		player_state->pixel_x -= player_state->speed;
		break;
	default:
		break;
	}

	player_state->move_counter -= player_state->speed;
}

static bool check_and_prep_screen_transition(WorldState *world_state,
					     PlayerState *player_state)
{
	TileMap *old_tile_map = world_state->current_tile_map;
	i32 tile_x            = player_state->tile_x;
	i32 tile_y            = player_state->tile_y;

	if (tile_y == -1 && old_tile_map->top_connection) {
		world_state->screen_transitioning = true;
		world_state->transition_counter   = SCREEN_HEIGHT_PIXELS;
		world_state->transition_direction = UPDIR;
		world_state->next_tile_map = old_tile_map->top_connection;
		return true;
	} else if (tile_y == SCREEN_HEIGHT_TILES &&
		   old_tile_map->bottom_connection) {
		world_state->screen_transitioning = true;
		world_state->transition_counter   = SCREEN_HEIGHT_PIXELS;
		world_state->transition_direction = DOWNDIR;
		world_state->next_tile_map = old_tile_map->bottom_connection;
		return true;
	} else if (tile_x == -1 && old_tile_map->left_connection) {
		world_state->screen_transitioning = true;
		world_state->transition_counter   = SCREEN_WIDTH_PIXELS;
		world_state->transition_direction = LEFTDIR;
		world_state->next_tile_map = old_tile_map->left_connection;
		return true;
	} else if (tile_x == SCREEN_WIDTH_TILES &&
		   old_tile_map->right_connection) {
		world_state->screen_transitioning = true;
		world_state->transition_counter   = SCREEN_WIDTH_PIXELS;
		world_state->transition_direction = RIGHTDIR;
		world_state->next_tile_map = old_tile_map->right_connection;
		return true;
	}

	return false;
}

static void play_sound(Sound *game_sound)
{
	if (game_sound->sound_initialized && game_sound->sound_playing) {
		i32 rc = debug_platform_stream_audio("resources/test.wav",
						     game_sound);
		if (rc <= 0) {
			game_sound->sound_playing = false;
		}
	}
}

static void render_player(i32 *image_buffer, PlayerState *player_state)
{
	i32 player_min_x = player_state->pixel_x - 16;
	i32 player_max_x = player_state->pixel_x + 17;
	i32 player_min_y = player_state->pixel_y - 16;
	i32 player_max_y = player_state->pixel_y + 17;

	i32 sprite_number = player_state->sprite_number;
	bool mirrored     = player_state->move_direction == LEFTDIR;

	if (player_state->sprite_location) {
		display_bitmap_tile(image_buffer, player_state->sprite_location,
				    sprite_number, player_min_x, player_min_y,
				    TILE_WIDTH, TILE_HEIGHT, mirrored);
	} else {
		render_rectangle(image_buffer, player_min_x, player_max_x,
				 player_min_y, player_max_y, 0.0, 0.8, 0.25);
	}
}

static void render_rectangle(i32 *image_buffer, i32 min_x, i32 max_x, i32 min_y,
			     i32 max_y, float red, float green, float blue)
{
	i32 clamped_min_x = min_x >= 0 ? min_x : 0;
	i32 clamped_min_y = min_y >= 0 ? min_y : 0;
	i32 clamped_max_x = max_x > WIN_WIDTH ? WIN_WIDTH : max_x;
	i32 clamped_max_y = max_y > WIN_HEIGHT ? WIN_HEIGHT : max_y;

	i32 int_red   = (i32)(red * 255.0f);
	i32 int_green = (i32)(green * 255.0f);
	i32 int_blue  = (i32)(blue * 255.0f);

	i32 color = (int_red << 16) | (int_green << 8) | int_blue;

	for (int y = clamped_min_y; y < clamped_max_y; y++) {
		for (int x = clamped_min_x; x < clamped_max_x; x++) {
			image_buffer[y * WIN_WIDTH + x] = color;
		}
	}
};

static void render_status_bar(i32 *image_buffer)
{
	render_rectangle(image_buffer, 0, 1280, 640, 720, 1.0f, 0.0f, 1.0f);
}

static void render_tile_map(i32 *image_buffer, TileMap *tile_map,
			    void *tile_set, i32 x_offset, i32 y_offset)
{
	if (!tile_set | !tile_map | !image_buffer)
		return;

	/* TODO: Don't bother trying to draw tiles that are off screen */

	/* Background tiles */
	i32 *background_map = (i32 *)tile_map->background_map;
	for (i32 row = 0; row < SCREEN_HEIGHT_TILES; row++) {
		for (i32 column = 0; column < SCREEN_WIDTH_TILES; column++) {
			i32 target_y = row * TILE_HEIGHT;
			i32 target_x = column * TILE_WIDTH;
			i32 tile_number =
				background_map[row * SCREEN_WIDTH_TILES +
					       column];

			display_bitmap_tile(image_buffer, tile_set, tile_number,
					    target_x + x_offset,
					    target_y + y_offset, TILE_WIDTH,
					    TILE_HEIGHT, false);
		}
	}

	/* Foreground tiles */
	i32 *foreground_tiles = tile_map->foreground_tiles;
	i32 len               = tile_map->foreground_tiles_length;
	for (i32 i = 0; i < len; i++) {
		i32 tile_data   = foreground_tiles[i];
		i32 target_x    = (tile_data & 0xFF000000) >> 24;
		i32 target_y    = (tile_data & 0x00FF0000) >> 16;
		i32 tile_number = (tile_data & 0x0000FFFF);

		target_x *= TILE_WIDTH;
		target_y *= TILE_HEIGHT;

		display_bitmap_tile(image_buffer, tile_set, tile_number,
				    target_x + x_offset, target_y + y_offset,
				    TILE_WIDTH, TILE_HEIGHT, false);
	}
}

static void set_tile_map_value(i32 x, i32 y, i32 layer, i32 tile_number,
			       TileMap *tile_map)
{
	switch (layer) {
	case 0:
		tile_map->background_map[y][x] = tile_number;
		break;
	case 1: {

		i32 len = tile_map->foreground_tiles_length;
		tile_map->foreground_tiles[len] = ((x & 0xFF) << 24) |
			((y & 0xFF) << 16) | (tile_number & 0xFFFF);
		tile_map->foreground_tiles_length += 1;
		break;
	}
	case 2:
		tile_map->collision_map[y][x] = !!tile_number;
		break;
	default:
		break;
	}
}

static void transition_screens(i32 *image_buffer, PlayerState *player_state,
			       WorldState *world_state)
{
	Direction transition_direction = world_state->transition_direction;

	i32 transition_speed_y = 16;
	i32 transition_speed_x = 20;

	/* If we're done transitioning... */
	if (world_state->transition_counter <= 0) {
		world_state->screen_transitioning = false;
		world_state->current_tile_map     = world_state->next_tile_map;

		/* Put player in correct spot on new map */
		switch (transition_direction) {
		case UPDIR:
			player_state->tile_y += SCREEN_HEIGHT_TILES;
			player_state->pixel_y = convert_tile_to_pixel(
				player_state->tile_y, Y_DIMENSION);
			break;
		case DOWNDIR:
			player_state->tile_y -= SCREEN_HEIGHT_TILES;
			player_state->pixel_y = convert_tile_to_pixel(
				player_state->tile_y, Y_DIMENSION);
			break;
		case RIGHTDIR:
			player_state->tile_x -= SCREEN_WIDTH_TILES;
			player_state->pixel_x = convert_tile_to_pixel(
				player_state->tile_x, X_DIMENSION);
			break;
		case LEFTDIR:
			player_state->tile_x += SCREEN_WIDTH_TILES;
			player_state->pixel_x = convert_tile_to_pixel(
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
		new_map_y_offset = old_map_y_offset - SCREEN_HEIGHT_PIXELS;
		player_state->pixel_y += transition_speed_y;
		break;
	case DOWNDIR:
		old_map_y_offset =
			world_state->transition_counter - SCREEN_HEIGHT_PIXELS;
		new_map_y_offset = SCREEN_HEIGHT_PIXELS + old_map_y_offset;
		player_state->pixel_y -= transition_speed_y;
		break;
	case RIGHTDIR:
		old_map_x_offset =
			world_state->transition_counter - SCREEN_WIDTH_PIXELS;
		new_map_x_offset = SCREEN_WIDTH_PIXELS + old_map_x_offset;
		player_state->pixel_x -= transition_speed_x;
		break;
	case LEFTDIR:
		old_map_x_offset =
			SCREEN_WIDTH_PIXELS - world_state->transition_counter;
		new_map_x_offset = old_map_x_offset - SCREEN_WIDTH_PIXELS;
		player_state->pixel_x += transition_speed_x;
		break;
	default:
		break;
	}

	/* TODO: Refactor this so that it doesn't require two tile map draws? */
	render_tile_map(image_buffer, world_state->next_tile_map,
			world_state->tile_set, new_map_x_offset,
			new_map_y_offset);

	render_tile_map(image_buffer, world_state->current_tile_map,
			world_state->tile_set, old_map_x_offset,
			old_map_y_offset);

	render_player(image_buffer, player_state);

	render_status_bar(image_buffer);
}
