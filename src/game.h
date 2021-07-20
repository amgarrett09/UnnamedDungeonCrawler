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
 * Dependencies: <stdint.h>, <stdbool.h>
 */

#define WIN_X 10
#define WIN_Y 10
#define WIN_WIDTH 1280
#define WIN_HEIGHT 720
#define WIN_BORDER 1
#define TILE_WIDTH 32
#define TILE_HEIGHT 32
#define SCREEN_WIDTH_TILES 40
#define SCREEN_HEIGHT_TILES 20

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#pragma pack(push, 1)
typedef struct {
	u16 signature;
	u32 file_size;
	u16 reserved1;
	u16 reserved2;
	u32 image_offset;
	u32 info_header_size;
	i32 image_width;
	i32 image_height;
	u16 number_of_planes;
	u16 bits_per_pixel;
	u32 compression_type;
	u32 image_data_size;
	i32 horizontal_resolution;
	i32 vertical_resolution;
	u32 colors_used;
	u32 important_colors;
	u32 red_mask;
	u32 green_mask;
	u32 blue_mask;
	u32 alpha_mask;
} BMPHeader;
#pragma pack(pop)

typedef struct {
	i32 width;
	i32 height;
	char data[];
} Bitmap;

typedef enum { X_DIMENSION, Y_DIMENSION } CoordDimension;

typedef enum { NULLDIR, UPDIR, RIGHTDIR, DOWNDIR, LEFTDIR } Direction;

typedef enum { KEY_NULL, KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_LEFT } Key;

typedef enum {
	KEYMASK_UP    = 0x01,
	KEYMASK_RIGHT = 0x02,
	KEYMASK_DOWN  = 0x04,
	KEYMASK_LEFT  = 0x08
} KeyMask;

typedef struct {
	i32 keys;
} Input;

typedef struct {
	void *sprite_location;
	i32 sprite_number;
	i32 pixel_x;
	i32 pixel_y;
	i32 tile_x;
	i32 tile_y;
	i32 move_counter;
	Direction move_direction;
	i32 speed;
} PlayerState;

typedef struct TileMap {
	i32 background_map[SCREEN_HEIGHT_TILES][SCREEN_WIDTH_TILES];
	/*
	 * There usually won't be a screen full of foreground tiles, so
	 * to avoid looping through whole screen size, this is a 1D array with
	 * values represented like such:
	 * (x_coord << 24) | (y_coord << 16) | tile_number
	 */
	i32 foreground_tiles[SCREEN_HEIGHT_TILES * SCREEN_WIDTH_TILES];
	i32 collision_map[SCREEN_HEIGHT_TILES][SCREEN_WIDTH_TILES];
	struct TileMap *top_connection;
	struct TileMap *right_connection;
	struct TileMap *bottom_connection;
	struct TileMap *left_connection;
	i32 foreground_tiles_length;
} TileMap;

typedef struct {
	TileMap *current_tile_map;
	TileMap *next_tile_map;
	void *tile_set;
	bool screen_transitioning;
	Direction transition_direction;
	i32 transition_counter;
} WorldState;

typedef struct {
	PlayerState player_state;
	WorldState world_state;
	TileMap tile_maps[64];
	void *temp_storage;
	size_t temp_storage_size;
	size_t temp_next_load_offset;
	bool is_initialized;
} Memory;

typedef struct {
	PlayerState *player_state;
	WorldState *world_state;
	TileMap *tile_maps;
} MemoryPartitions;

typedef struct {
	char *sound_buffer;
	FILE *stream;
	size_t sound_buffer_size;
	bool sound_initialized;
	bool sound_playing;
} Sound;

void game_initialize_memory(Memory *memory, i32 dt);
void game_update_and_render(Memory *memory, Input *input, Sound *game_sound,
			    i32 *image_buffer);

/*
 * These are defined by platform layer.
 * We'll probably use a different solution later on.
 */
i32 debug_platform_stream_audio(const char file_path[], Sound *game_sound);

size_t debug_platform_load_asset(const char file_path[], void *memory_location);
