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
 * Dependencies: <stdint.h>, <stdbool.h>, hashmap.c
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
#define MAX_MAP_SEGMENTS 64
#define SAMPLES_PER_SECOND 44100
#define BYTES_PER_SAMPLE 4
#define TARGET_FRAME_RATE 60
#define MAX_PLAYER_SPRITE_SIZE 100 * 1024
#define MAX_SEGMENT_ENTITIES 50

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
	i32 sprite_number;
	i32 pixel_x;
	i32 pixel_y;
	i32 tile_x;
	i32 tile_y;
	i32 move_counter;
	Direction move_direction;
	i32 speed;
	char player_sprites[MAX_PLAYER_SPRITE_SIZE];
} PlayerState;

typedef struct Vec2 {
	i32 x;
	i32 y;
} Vec2;

typedef struct Entities {
	Vec2 positions[MAX_SEGMENT_ENTITIES];
	i32 num_entities;
} Entities;

typedef struct MapSegment {
	i32 index;
	struct MapSegment *top_connection;
	struct MapSegment *right_connection;
	struct MapSegment *bottom_connection;
	struct MapSegment *left_connection;
	/* Format for tiles: (bg_tile_num << 16) | fg_tile_num */
	i32 tiles[SCREEN_HEIGHT_TILES][SCREEN_WIDTH_TILES];
	struct Entities entities;
} MapSegment;

typedef enum {
	TRANS_STATE_NORMAL = 0,
	TRANS_STATE_WAITING,
	TRANS_STATE_SCROLLING,
	TRANS_STATE_WARPING
} TransitionState;

typedef struct {
	MapSegment *current_map_segment;
	MapSegment *next_map_segment;
	void *tile_set;
	TransitionState trans_state;
	Direction transition_direction;
	i32 transition_counter;
	IntHashMap tile_props;
} WorldState;

typedef struct {
	PlayerState player_state;
	WorldState world_state;
	MapSegment map_segments[MAX_MAP_SEGMENTS];
	void *temp_storage;
	size_t temp_storage_size;
	size_t temp_next_load_offset;
	bool is_initialized;
} Memory;

typedef struct {
	/* Format for hot_tiles: (tile_x << 16) | tile_y */
	i32 hot_tiles[SCREEN_HEIGHT_TILES * SCREEN_WIDTH_TILES];
	i32 *image_buffer;
	i32 hot_tiles_length;
} ScreenState;

typedef struct {
	void *sound_buffer;
	i32 sound_buffer_size;
	bool playing;
} Sound;

typedef struct {
	FILE *fd;
} FileStream;

void game_initialize_memory(Memory *memory, ScreenState *screen_state, i32 dt);
void game_update_and_render(Memory *memory, Input *input,
			    ScreenState *screen_state);

/*
 * These are defined by platform layer.
 */
i32 debug_platform_stream_audio(const char file_path[], FileStream *stream,
				void *sound_buffer, i32 sound_buffer_size);
size_t debug_platform_load_asset(const char file_path[], void *memory_location,
				 size_t max_size);
