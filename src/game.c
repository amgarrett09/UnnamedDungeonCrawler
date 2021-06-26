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
 * Dependendencies: game.h, tile_maps.c
 */



static const i32 SCREEN_HEIGHT_PIXELS = SCREEN_HEIGHT_TILES * TILE_HEIGHT;
static const i32 SCREEN_WIDTH_PIXELS = SCREEN_WIDTH_TILES * TILE_WIDTH;

static i32 bit_scan_forward_u(u32 number);
static i32 convert_tile_to_pixel(i32 tile_value, CoordDimension dimension);
static void display_bitmap_tile(i32 *restrict image_buffer, BMPHeader *bmp, 
                                i32 tile_number, i32 target_x, i32 target_y,
                                i32 tile_width, i32 tile_height, 
                                bool mirrored);

static size_t get_next_aligned_offset(size_t start_offset, size_t min_to_add, 
                                      size_t alignment);
static size_t load_bitmap(const char file_path[], void *location);
static void render_player(i32 *restrict image_buffer, 
                          PlayerState *restrict player_state);
static void render_rectangle(i32 *image_buffer, i32 min_x, i32 max_x, i32 min_y,
                             i32 max_y, float red, float green, float blue);
static void render_status_bar(i32 *image_buffer);
static void render_tile_map(i32 *restrict image_buffer, i32 *restrict tile_map,
                            void *restrict tile_set, i32 x_offset, 
                            i32 y_offset);
static void transition_screens(i32 *restrict image_buffer,
                               PlayerState *restrict player_state,
                               WorldState *restrict world_state);


void 
game_initialize_memory(Memory *memory, i32 dt) 
{
        MemoryPartitions *partitions = 
            (MemoryPartitions *) memory->perm_storage;


        partitions->player_state = (PlayerState *) 
            get_next_aligned_offset((size_t)partitions, 
                                    sizeof(MemoryPartitions),
                                    32);
        partitions->world_state = (WorldState *) 
            get_next_aligned_offset((size_t)partitions->player_state,
                                    sizeof(PlayerState),
                                    32);
                                                   

        PlayerState *player_state = partitions->player_state;
        WorldState *world_state = partitions->world_state;

        size_t sprite_load_result = 
            load_bitmap("resources/player_sprites.bmp", memory->temp_storage);
        if (sprite_load_result) {
                player_state->sprite_location = memory->temp_storage;
                memory->temp_next_load_offset = 
                    get_next_aligned_offset(memory->temp_next_load_offset,
                                            sprite_load_result, 32);
        }

        char *next_load_location = 
            (char *) memory->temp_storage + memory->temp_next_load_offset;
        size_t tile_load_result = 
            load_bitmap("resources/tile_set.bmp", 
                        (void *)next_load_location);
        if (tile_load_result) {
                world_state->tile_set = (void *)next_load_location;
                memory->temp_next_load_offset = 
                    get_next_aligned_offset(memory->temp_next_load_offset,
                                            tile_load_result, 32);
        }

        player_state->tile_x = 15;
        player_state->tile_y = 3;
        player_state->pixel_x = 
            convert_tile_to_pixel(player_state->tile_x, X_DIMENSION);
        player_state->pixel_y = 
            convert_tile_to_pixel(player_state->tile_y, Y_DIMENSION);
        player_state->move_counter = 0;
        player_state->move_direction = NULLDIR;
        player_state->speed = ((TILE_WIDTH / 8) * dt) / 16;
        player_state->sprite_number = 1;

        tile_map1.top_connection = &tile_map2;
        tile_map1.right_connection = &tile_map3;
        tile_map1.bottom_connection = &tile_map4;
        tile_map1.left_connection = &tile_map5;

        tile_map2.bottom_connection = &tile_map1;
        tile_map3.left_connection = &tile_map1;
        tile_map4.top_connection = &tile_map1;
        tile_map5.right_connection = &tile_map1;

        world_state->current_tile_map = &tile_map1;
        world_state->next_tile_map = &tile_map1;
        world_state->screen_transitioning = false;
        world_state->transition_direction = NULLDIR;
        world_state->transition_counter = 0;

        memory->is_initialized = true;
}

void 
game_update_and_render(Memory *restrict memory, Input *restrict input, 
                       Sound *restrict game_sound, i32 *restrict image_buffer,
                       i32 dt) 
{
        MemoryPartitions *partitions = 
            (MemoryPartitions *) memory->perm_storage;
        PlayerState *player_state = partitions->player_state;
        WorldState *world_state = partitions->world_state;
        
        if (world_state->screen_transitioning) {
                transition_screens(image_buffer, player_state, world_state);
                return;
        }

        /* Read input */
        switch (input->key_pressed) {
                case UPKEY:
                        player_state->keys = player_state->keys | UP_MASK;
                        break;
                case RIGHTKEY:
                        player_state->keys = player_state->keys | RIGHT_MASK;
                        break;
                case DOWNKEY:
                        player_state->keys = player_state->keys | DOWN_MASK;
                        break;
                case LEFTKEY:
                        player_state->keys = player_state->keys | LEFT_MASK;
                        break;
                default:
                        break;
        }
        switch (input->key_released) {
                case UPKEY:
                        player_state->keys = player_state->keys & (~UP_MASK);
                        break;
                case RIGHTKEY:
                        player_state->keys = player_state->keys & (~RIGHT_MASK);
                        break;
                case DOWNKEY:
                        player_state->keys = player_state->keys & (~DOWN_MASK);
                        break;
                case LEFTKEY:
                        player_state->keys = player_state->keys & (~LEFT_MASK);
                        break;
                default:
                        break;
        }


        /* If we're at the end of a move... */
        i32 keys = player_state->keys;
        if (player_state->move_counter <= 0) {
                /* Check for screen transition */
                TileMap *old_tile_map = world_state->current_tile_map;
                i32 tile_x = player_state->tile_x;
                i32 tile_y = player_state->tile_y;

                if (tile_y == -1 && old_tile_map->top_connection) {
                        world_state->screen_transitioning = true;
                        world_state->transition_counter = SCREEN_HEIGHT_PIXELS;
                        world_state->transition_direction = UPDIR;
                        world_state->next_tile_map = 
                            old_tile_map->top_connection;
                        return;
                } else if (tile_y == SCREEN_HEIGHT_TILES 
                           && old_tile_map->bottom_connection) {
                        world_state->screen_transitioning = true;
                        world_state->transition_counter = SCREEN_HEIGHT_PIXELS;
                        world_state->transition_direction = DOWNDIR;
                        world_state->next_tile_map = 
                            old_tile_map->bottom_connection;
                        return;
                } else if (tile_x == -1 && old_tile_map->left_connection) {
                        world_state->screen_transitioning = true;
                        world_state->transition_counter = SCREEN_WIDTH_PIXELS;
                        world_state->transition_direction = LEFTDIR;
                        world_state->next_tile_map = 
                            old_tile_map->left_connection;
                        return;
                } else if (tile_x == SCREEN_WIDTH_TILES 
                           && old_tile_map->right_connection) {
                        world_state->screen_transitioning = true;
                        world_state->transition_counter = SCREEN_WIDTH_PIXELS;
                        world_state->transition_direction = RIGHTDIR;
                        world_state->next_tile_map = 
                            old_tile_map->right_connection;
                        return;
                }

                /* Movement and collision detection */
                i32 *current_collision_map = 
                    (i32 *) world_state->current_tile_map->collision_map;
                if (keys & UP_MASK || keys & DOWN_MASK) {
                        bool is_up = !(keys & DOWN_MASK);
                        i32 change = is_up ? -1 : 1;
                        i32 new_tile_y = player_state->tile_y + change;
                        i32 tile_x = player_state->tile_x;

                        player_state->sprite_number = is_up ? 5 : 1;
                        player_state->move_direction = is_up ? UPDIR : DOWNDIR;

                        bool is_not_colliding = true;

                        if (new_tile_y >= 0 && new_tile_y < SCREEN_HEIGHT_TILES
                            && tile_x >= 0
                            && tile_x < SCREEN_WIDTH_TILES) {
                                is_not_colliding = !current_collision_map[
                                    new_tile_y*SCREEN_WIDTH_TILES + tile_x];
                        }

                        if (is_not_colliding) {
                                player_state->tile_y = new_tile_y;
                                player_state->move_counter = TILE_HEIGHT;
                        }
                } else if (keys & RIGHT_MASK || keys & LEFT_MASK) {
                        bool is_right = !(keys & LEFT_MASK);
                        i32 change = is_right ? 1 : -1;
                        i32 new_tile_x = player_state->tile_x + change;
                        i32 tile_y = player_state->tile_y;

                        player_state->sprite_number = 9;
                        player_state->move_direction = 
                            is_right ? RIGHTDIR : LEFTDIR;

                        bool is_not_colliding = true;

                        if (new_tile_x >= 0 && new_tile_x < SCREEN_WIDTH_TILES 
                            && tile_y >= 0 
                            && tile_y < SCREEN_HEIGHT_TILES) {
                                is_not_colliding = !current_collision_map[
                                    tile_y*SCREEN_WIDTH_TILES + new_tile_x];
                        }

                        if (is_not_colliding) {
                                player_state->tile_x = new_tile_x;
                                player_state->move_counter = TILE_WIDTH;
                        }
                }
        }

        /* If we're completing a movement command... */ 
        if (player_state->move_counter > 0) {
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


        /* Render */
        render_tile_map(image_buffer, 
                        (i32 *) world_state->current_tile_map->tile_map, 
                        world_state->tile_set, 0, 0);

        render_player(image_buffer, player_state);

        render_status_bar(image_buffer);
}

/*
 * Finds first set bit in an unsigned integer, starting from lowest bit.
 * Returns the index of the set bit.
 */
static i32 
bit_scan_forward_u(u32 number) 
{
        /* TODO: Use intrinsic for this */
        bool found = false;
        i32 index = 0;

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

static i32 
convert_tile_to_pixel(i32 tile_value, CoordDimension dimension) 
{
        if (dimension == Y_DIMENSION) {
                return TILE_HEIGHT * tile_value + (TILE_HEIGHT / 2);
        } else {
                return TILE_WIDTH * tile_value + (TILE_WIDTH / 2);
        }
}

static void
display_bitmap_tile(i32 *restrict image_buffer, BMPHeader *bmp, 
                    i32 tile_number, i32 target_x, i32 target_y,
                    i32 tile_width, i32 tile_height, bool mirrored) 
{
        u32 image_offset = bmp->image_offset;
        i32 bmp_width = bmp->image_width;
        i32 bmp_height = bmp->image_height;

        /* Calculate starting x and y position in bmp based on tile number */
        i32 source_x = (tile_number * tile_width) % bmp_width;
        i32 source_y = (tile_number / (bmp_width / tile_width)) * tile_height; 

        char *image_start = ((char *) bmp) + image_offset;
        i32 *restrict image = (i32 *) image_start;

        /* BMP pixels are arranged bottom to top */
        i32 bmp_row_start = (bmp_height - 1) * bmp_width;
        bmp_row_start -= source_y * bmp_width;

        /* 
         * Make sure following loop never goes out of bounds 
         * (also clips tile at screen edge)
         */
        i32 start_row = 0;
        i32 end_row = tile_height;
        i32 start_column = 0;
        i32 end_column = tile_width;

        if (target_y >= WIN_HEIGHT - tile_height) {
                end_row -= target_y - (WIN_HEIGHT - tile_height);
        } else if (target_y < 0) {
                start_row -= target_y;
                bmp_row_start += target_y*bmp_width;
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
                        i32 target_pixel = target_row*WIN_WIDTH + target_column;

                        i32 bmp_color  = image[source_pixel];
                        i32 buffer_color = 
                            image_buffer[target_pixel];

                        /* Linear Alpha Blend bmp with existing data in buffer*/
                        i32 alpha = (bmp_color & 0xff000000) >> 24;
                        float opacity = alpha / 255.0f;

                        i32 bmp_red = (bmp_color & 0x00ff0000) >> 16;
                        i32 bmp_green = (bmp_color & 0x0000ff00) >> 8;
                        i32 bmp_blue = (bmp_color & 0x000000ff);

                        i32 buffer_red = (buffer_color & 0x00ff0000) >> 16;
                        i32 buffer_green = (buffer_color & 0x0000ff00) >> 8;
                        i32 buffer_blue = (buffer_color & 0x000000ff);

                        float new_red = 
                            ((1 - opacity)*buffer_red) + (opacity*bmp_red);
                        float new_green = 
                            ((1 - opacity)*buffer_green) + (opacity*bmp_green);
                        float new_blue = 
                            ((1 - opacity)*buffer_blue) + (opacity*bmp_blue);

                        i32 new_color = 
                            ((i32)new_red << 16) 
                            | ((i32)new_green << 8) 
                            | (i32)new_blue;

                        image_buffer[target_pixel] = new_color;
                }

                bmp_row_start -= bmp_width;
        }
}

static size_t
get_next_aligned_offset(size_t start_offset, size_t min_to_add, 
                        size_t alignment)
{
        size_t min_new_offset = start_offset + min_to_add;
        size_t remainder = min_new_offset % alignment;

        if (remainder) {
            return min_new_offset + (alignment - remainder);
        } else {
            return min_new_offset;
        }
}

static size_t 
load_bitmap(const char file_path[], void *location) 
{
        size_t result = debug_platform_load_asset(file_path, location);

        if (result == 0) {
                return 0;
        }

        BMPHeader *bmp = (BMPHeader *) location;

        /* Put an accurate file size in the header */
        bmp->file_size = (u32) result;

        u32 image_offset = bmp->image_offset;
        i32 image_width = bmp->image_width;
        i32 image_height = bmp->image_height;

        u32 red_mask = bmp->red_mask;
        u32 green_mask = bmp->green_mask;
        u32 blue_mask = bmp->blue_mask;
        u32 alpha_mask = bmp->alpha_mask;

        /* How many bits we need to shift to get values to start at bit 0*/
        i32 red_shift = bit_scan_forward_u(red_mask);
        i32 green_shift = bit_scan_forward_u(green_mask);
        i32 blue_shift = bit_scan_forward_u(blue_mask);
        i32 alpha_shift = bit_scan_forward_u(alpha_mask);

        red_shift = red_shift < 0 ? 0 : red_shift;
        green_shift = green_shift < 0 ? 0 : green_shift;
        blue_shift = blue_shift < 0 ? 0 : blue_shift;
        alpha_shift = alpha_shift < 0 ? 0 : alpha_shift;

        char *image_start = ((char *) bmp) + image_offset;
        i32 *image = (i32 *) image_start;

        for (i32 i = 0; i < image_width*image_height; i++) {
                i32 color = image[i]; 

                /* Swizzle color values to ensure ARGB order */
                i32 alpha = 
                    (((color & alpha_mask) >> alpha_shift) & 0xFF) << 24;
                i32 red = (((color & red_mask) >> red_shift) & 0xFF) << 16;
                i32 green = (((color & green_mask) >> green_shift) & 0xFF) << 8;
                i32 blue = (((color & blue_mask) >> blue_shift) & 0xFF);

                image[i] = alpha | red | green | blue;
        }

        return result;
}

static void
render_player(i32 *restrict image_buffer, PlayerState *restrict player_state) 
{
        i32 player_min_x = player_state->pixel_x - 16;
        i32 player_max_x = player_state->pixel_x + 17;
        i32 player_min_y = player_state->pixel_y - 16;
        i32 player_max_y = player_state->pixel_y + 17;

        i32 sprite_number = player_state->sprite_number;
        bool mirrored = player_state->move_direction == LEFTDIR;

        if (player_state->sprite_location) {
                display_bitmap_tile(image_buffer, player_state->sprite_location,
                                    sprite_number, player_min_x, player_min_y, 
                                    TILE_WIDTH, TILE_HEIGHT, mirrored);
        } else {
                render_rectangle(image_buffer, player_min_x, player_max_x, 
                                 player_min_y, player_max_y, 0.0, 0.8, 0.25);
        }
}

static void 
render_rectangle(i32 *image_buffer, i32 min_x, i32 max_x, i32 min_y, i32 max_y,
                 float red, float green, float blue) 
{
        i32 clamped_min_x = min_x >= 0 ? min_x : 0;
        i32 clamped_min_y = min_y >= 0 ? min_y : 0;
        i32 clamped_max_x = max_x > WIN_WIDTH ? WIN_WIDTH : max_x;
        i32 clamped_max_y = max_y > WIN_HEIGHT ? WIN_HEIGHT : max_y;

        i32 int_red = (i32) (red * 255.0f);
        i32 int_green = (i32) (green * 255.0f);
        i32 int_blue = (i32) (blue * 255.0f);

        i32 color = (int_red << 16) | (int_green << 8) | int_blue;

        for (int y = clamped_min_y; y < clamped_max_y; y++) {
                for (int x = clamped_min_x; x < clamped_max_x; x++) {
                        image_buffer[y*WIN_WIDTH + x] = color;
                }
        }
};

static void
render_status_bar(i32 *image_buffer)
{
        render_rectangle(image_buffer, 0, 1280, 640, 720, 1.0f, 0.0f, 1.0f);
}

static void 
render_tile_map(i32 *restrict image_buffer, i32 *restrict tile_map,
                void *restrict tile_set, i32 x_offset, i32 y_offset) 
{
        if (!tile_set | !tile_map | !image_buffer) 
            return;

        /* TODO: Don't bother trying to draw tiles that are off screen */
        for (i32 row = 0; row < SCREEN_HEIGHT_TILES; row++) {
                for (i32 column = 0; column < SCREEN_WIDTH_TILES; column++) {
                        i32 target_y = row*TILE_HEIGHT;
                        i32 target_x = column*TILE_WIDTH;
                        i32 tile_number = 
                            tile_map[row*SCREEN_WIDTH_TILES + column];

                        display_bitmap_tile(image_buffer, 
                                            tile_set, 
                                            tile_number, 
                                            target_x + x_offset, 
                                            target_y + y_offset,
                                            TILE_WIDTH, TILE_HEIGHT, 
                                            false);
                }
        }
}

static void 
transition_screens(i32 *restrict image_buffer, 
                   PlayerState *restrict player_state,
                   WorldState *restrict world_state) 
{
        Direction transition_direction = world_state->transition_direction;

        i32 transition_speed_y = 16;
        i32 transition_speed_x = 20;

        /* If we're done transitioning... */
        if (world_state->transition_counter <= 0) {
                world_state->screen_transitioning = false;
                world_state->current_tile_map = world_state->next_tile_map;

                player_state->keys = 0;

                /* Put player in correct spot on new map */
                switch (transition_direction) {
                        case UPDIR:
                                player_state->tile_y += SCREEN_HEIGHT_TILES;
                                player_state->pixel_y = 
                                    convert_tile_to_pixel(player_state->tile_y, 
                                                          Y_DIMENSION);
                                break;
                        case DOWNDIR:
                                player_state->tile_y -= SCREEN_HEIGHT_TILES;
                                player_state->pixel_y = 
                                    convert_tile_to_pixel(player_state->tile_y, 
                                                          Y_DIMENSION);
                                break;
                        case RIGHTDIR:
                                player_state->tile_x -= SCREEN_WIDTH_TILES;
                                player_state->pixel_x = 
                                    convert_tile_to_pixel(player_state->tile_x, 
                                                          X_DIMENSION);
                                break;
                        case LEFTDIR:
                                player_state->tile_x += SCREEN_WIDTH_TILES;
                                player_state->pixel_x = 
                                    convert_tile_to_pixel(player_state->tile_x, 
                                                          X_DIMENSION);
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
                            SCREEN_HEIGHT_PIXELS 
                            - world_state->transition_counter;
                        new_map_y_offset = 
                            old_map_y_offset - SCREEN_HEIGHT_PIXELS;
                        player_state->pixel_y += transition_speed_y; 
                        break;
                case DOWNDIR:
                        old_map_y_offset = 
                            world_state->transition_counter 
                            - SCREEN_HEIGHT_PIXELS;
                        new_map_y_offset = 
                            SCREEN_HEIGHT_PIXELS + old_map_y_offset;
                        player_state->pixel_y -= transition_speed_y; 
                        break;
                case RIGHTDIR:
                        old_map_x_offset = 
                            world_state->transition_counter 
                            - SCREEN_WIDTH_PIXELS;
                        new_map_x_offset = 
                            SCREEN_WIDTH_PIXELS + old_map_x_offset;
                        player_state->pixel_x -= transition_speed_x; 
                        break;
                case LEFTDIR:
                        old_map_x_offset = 
                            SCREEN_WIDTH_PIXELS 
                            - world_state->transition_counter;
                        new_map_x_offset = 
                            old_map_x_offset - SCREEN_WIDTH_PIXELS;
                        player_state->pixel_x += transition_speed_x; 
                        break;
                default:
                        break;
        }

        /* TODO: Refactor this so that it doesn't require two tile map draws? */
        render_tile_map(image_buffer, 
                        (i32 *) world_state->next_tile_map->tile_map,
                        world_state->tile_set,
                        new_map_x_offset, new_map_y_offset);

        render_tile_map(image_buffer, 
                        (i32 *) world_state->current_tile_map->tile_map,
                        world_state->tile_set,
                        old_map_x_offset, old_map_y_offset);

        render_player(image_buffer, player_state);

        render_status_bar(image_buffer);

}

